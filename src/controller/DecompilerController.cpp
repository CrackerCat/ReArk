#include "controller/DecompilerController.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QtConcurrent>

DecompilerController::DecompilerController(QObject* parent)
    : QObject(parent)
    , treeModel_(this)
    , tabsModel_(this)
{
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::selectedContentChanged);
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::selectedNameChanged);
    connect(&tabsModel_, &OpenFileTabsModel::activeTabChanged,
            this, &DecompilerController::diagnosticsChanged);
    connect(&treeModel_, &SourceTreeModel::selectedIndexChanged,
            this, &DecompilerController::selectedIndexChanged);
    connect(&treeModel_, &SourceTreeModel::fileActivated,
            this, &DecompilerController::openFileTab);
}

SourceTreeModel* DecompilerController::treeModel()
{
    return &treeModel_;
}

OpenFileTabsModel* DecompilerController::tabsModel()
{
    return &tabsModel_;
}

QString DecompilerController::selectedContent() const
{
    return tabsModel_.activeContent();
}

QString DecompilerController::selectedName() const
{
    return tabsModel_.activePath();
}

QString DecompilerController::diagnostics() const
{
    return tabsModel_.activeDiagnostics();
}

QString DecompilerController::status() const
{
    return status_;
}

bool DecompilerController::busy() const
{
    return busy_;
}

int DecompilerController::selectedIndex() const
{
    return treeModel_.selectedIndex();
}

void DecompilerController::decompileFile(const QString& filePath)
{
    ++openRequestId_;

    if (filePath.isEmpty()) {
        clear();
        return;
    }

    packageSession_.reset();
    packageSessionMutex_.reset();
    loadingNodes_.clear();
    const quint64 requestId = openRequestId_;
    setBusy(true);
    setStatus(tr("Decompiling %1").arg(QFileInfo(filePath).fileName()));

    auto* watcher = new QFutureWatcher<HyleDecompiler::OpenResult>(this);
    connect(watcher, &QFutureWatcher<HyleDecompiler::OpenResult>::finished, this, [this, watcher, requestId]() {
        applyOpenResult(requestId, watcher->result());
        watcher->deleteLater();
    });

    watcher->setFuture(QtConcurrent::run([filePath]() {
        return HyleDecompiler::openFile(filePath);
    }));
}

void DecompilerController::activateIndex(int index)
{
    treeModel_.activateIndex(index);
}

void DecompilerController::clear()
{
    ++openRequestId_;
    packageSession_.reset();
    packageSessionMutex_.reset();
    loadingNodes_.clear();
    tabsModel_.clear();
    treeModel_.replaceFiles({});
    setStatus(tr("Ready"));
    setBusy(false);
}

void DecompilerController::setSelectedIndex(int index)
{
    treeModel_.setSelectedIndex(index);
}

void DecompilerController::setStatus(const QString& status)
{
    if (status_ == status) {
        return;
    }
    status_ = status;
    emit statusChanged();
}

void DecompilerController::setBusy(bool busy)
{
    if (busy_ == busy) {
        return;
    }
    busy_ = busy;
    emit busyChanged();
}

void DecompilerController::applyOpenResult(quint64 requestId, HyleDecompiler::OpenResult result)
{
    if (requestId != openRequestId_) {
        return;
    }

    if (!result.error.isEmpty()) {
        packageSession_.reset();
        packageSessionMutex_.reset();
        loadingNodes_.clear();
        tabsModel_.clear();
        treeModel_.replaceFiles({});
        setStatus(result.error);
        setBusy(false);
        return;
    }

    packageSession_ = std::move(result.session);
    packageSessionMutex_ = std::move(result.sessionMutex);
    tabsModel_.clear();
    treeModel_.replaceFiles(std::move(result.files));
    if (loadingNodes_.empty()) {
        setBusy(false);
        setStatus(result.status);
    }
}

void DecompilerController::applySourceResult(quint64 requestId, HyleDecompiler::SourceResult result)
{
    if (requestId != openRequestId_) {
        return;
    }
    loadingNodes_.erase(result.nodeIndex);

    if (!result.error.isEmpty()) {
        treeModel_.markNodeFailed(result.nodeIndex, result.error);
        tabsModel_.updateNode(result.nodeIndex, result.error, QStringLiteral("text"), {});
    } else {
        treeModel_.setNodeContent(result.nodeIndex, result.content, result.diagnostics, result.kind, result.contentMode);
        tabsModel_.updateNode(result.nodeIndex, result.content, result.contentMode, result.diagnostics);
    }

    setStatus(result.error.isEmpty()
        ? tr("Loaded %1").arg(result.name)
        : result.error);
    setBusy(!loadingNodes_.empty());
}

void DecompilerController::openFileTab(int nodeIndex)
{
    tabsModel_.openOrActivate(
        nodeIndex,
        treeModel_.nodeName(nodeIndex),
        treeModel_.nodePath(nodeIndex),
        treeModel_.nodeContent(nodeIndex),
        treeModel_.nodeContentMode(nodeIndex),
        treeModel_.nodeDiagnostics(nodeIndex),
        treeModel_.nodeNeedsLoad(nodeIndex));

    if (treeModel_.nodeNeedsLoad(nodeIndex) && !loadingNodes_.contains(nodeIndex)) {
        loadingNodes_.insert(nodeIndex);
        setBusy(true);
        tabsModel_.setNodeLoading(nodeIndex, true);

        const quint64 requestId = openRequestId_;
        const auto hyleId = treeModel_.nodeHyleId(nodeIndex);
        const QString name = treeModel_.nodeName(nodeIndex);
        const QString section = treeModel_.nodeSection(nodeIndex);
        setStatus(section == QStringLiteral("resource")
            ? tr("Loading %1").arg(name)
            : tr("Decompiling %1").arg(name));
        const auto session = packageSession_;
        const auto sessionMutex = packageSessionMutex_;

        auto* watcher = new QFutureWatcher<HyleDecompiler::SourceResult>(this);
        connect(watcher, &QFutureWatcher<HyleDecompiler::SourceResult>::finished, this, [this, watcher, requestId]() {
            applySourceResult(requestId, watcher->result());
            watcher->deleteLater();
        });

        watcher->setFuture(QtConcurrent::run([session, sessionMutex, nodeIndex, hyleId, name, section]() {
            if (section == QStringLiteral("resource")) {
                return HyleDecompiler::readResourceContent(session, sessionMutex, nodeIndex, hyleId, name);
            }
            return HyleDecompiler::decompileSourceFile(session, sessionMutex, nodeIndex, hyleId, name);
        }));
    }
}
