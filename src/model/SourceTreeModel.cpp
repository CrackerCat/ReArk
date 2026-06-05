#include "model/SourceTreeModel.h"

#include "core/PerformanceTrace.h"

#include <algorithm>
#include <map>
#include <memory>
#include <QString>
#include <utility>

namespace {

QString sortName(const QString& name)
{
    return name.toCaseFolded();
}

std::shared_ptr<DocumentContent> makeDocument(QString text, QByteArray binary, QString diagnostics, QString kind, QString contentMode)
{
    auto document = std::make_shared<DocumentContent>();
    document->text = std::move(text);
    document->binary = std::move(binary);
    document->diagnostics = std::move(diagnostics);
    document->kind = std::move(kind);
    document->contentMode = std::move(contentMode);
    return document;
}

QString documentText(const std::shared_ptr<DocumentContent>& document)
{
    return document ? document->text : QString{};
}

QByteArray documentBinary(const std::shared_ptr<DocumentContent>& document)
{
    return document ? document->binary : QByteArray{};
}

QString documentDiagnostics(const std::shared_ptr<DocumentContent>& document)
{
    return document ? document->diagnostics : QString{};
}

} // namespace

SourceTreeModel::SourceTreeModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int SourceTreeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(visibleRows_.size());
}

QVariant SourceTreeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const auto& node = nodes_.at(static_cast<std::size_t>(visibleRows_.at(static_cast<std::size_t>(index.row()))));
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return node.name;
    case PathRole:
        return node.path;
    case KindRole:
        return node.kind;
    case ContentRole:
        return documentText(node.document);
    case DiagnosticsRole:
        return documentDiagnostics(node.document);
    case DepthRole:
        return node.depth;
    case DirectoryRole:
        return node.directory;
    case ExpandedRole:
        return node.expanded;
    case PlaceholderRole:
        return node.placeholder;
    default:
        return {};
    }
}

QHash<int, QByteArray> SourceTreeModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { PathRole, "path" },
        { KindRole, "kind" },
        { ContentRole, "content" },
        { DiagnosticsRole, "diagnostics" },
        { DepthRole, "depth" },
        { DirectoryRole, "isDirectory" },
        { ExpandedRole, "expanded" },
        { PlaceholderRole, "isPlaceholder" }
    };
}

QString SourceTreeModel::selectedContent() const
{
    if (selectedNode_ < 0 || selectedNode_ >= static_cast<int>(nodes_.size())) {
        return tr("// Drop a package to start decompiling.");
    }
    const auto& node = nodes_.at(static_cast<std::size_t>(selectedNode_));
    if (node.directory) {
        return tr("// Select a source file.");
    }
    if (node.lazy && !node.document) {
        return tr("// Decompiling selected source file...");
    }
    return documentText(node.document);
}

QString SourceTreeModel::selectedName() const
{
    if (selectedNode_ < 0 || selectedNode_ >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return nodes_.at(static_cast<std::size_t>(selectedNode_)).path;
}

QString SourceTreeModel::diagnostics() const
{
    if (selectedNode_ < 0 || selectedNode_ >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return documentDiagnostics(nodes_.at(static_cast<std::size_t>(selectedNode_)).document);
}

int SourceTreeModel::selectedIndex() const
{
    return rowForNode(selectedNode_);
}

int SourceTreeModel::selectedNode() const
{
    return selectedNode_;
}

bool SourceTreeModel::selectedNeedsLoad() const
{
    if (selectedNode_ < 0 || selectedNode_ >= static_cast<int>(nodes_.size())) {
        return false;
    }
    const auto& node = nodes_.at(static_cast<std::size_t>(selectedNode_));
    return !node.directory && !node.placeholder && node.lazy && !node.document;
}

std::size_t SourceTreeModel::selectedHyleId() const
{
    if (selectedNode_ < 0 || selectedNode_ >= static_cast<int>(nodes_.size())) {
        return 0;
    }
    return nodes_.at(static_cast<std::size_t>(selectedNode_)).hyleId;
}

QString SourceTreeModel::selectedFileName() const
{
    if (selectedNode_ < 0 || selectedNode_ >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return nodes_.at(static_cast<std::size_t>(selectedNode_)).name;
}

QString SourceTreeModel::nodeContent(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return documentText(nodes_.at(static_cast<std::size_t>(nodeIndex)).document);
}

QByteArray SourceTreeModel::nodeBinaryContent(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return documentBinary(nodes_.at(static_cast<std::size_t>(nodeIndex)).document);
}

QString SourceTreeModel::nodeDiagnostics(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return documentDiagnostics(nodes_.at(static_cast<std::size_t>(nodeIndex)).document);
}

std::size_t SourceTreeModel::nodeHyleId(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return 0;
    }
    return nodes_.at(static_cast<std::size_t>(nodeIndex)).hyleId;
}

QString SourceTreeModel::nodeName(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return nodes_.at(static_cast<std::size_t>(nodeIndex)).name;
}

QString SourceTreeModel::nodePath(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return nodes_.at(static_cast<std::size_t>(nodeIndex)).path;
}

QString SourceTreeModel::nodeKind(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return nodes_.at(static_cast<std::size_t>(nodeIndex)).kind;
}

std::shared_ptr<DocumentContent> SourceTreeModel::nodeDocument(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return nodes_.at(static_cast<std::size_t>(nodeIndex)).document;
}

QString SourceTreeModel::nodeSection(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return {};
    }
    return nodes_.at(static_cast<std::size_t>(nodeIndex)).section;
}

QString SourceTreeModel::nodeContentMode(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return QStringLiteral("text");
    }
    return nodes_.at(static_cast<std::size_t>(nodeIndex)).contentMode;
}

bool SourceTreeModel::nodeNeedsLoad(int nodeIndex) const
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return false;
    }
    const auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
    return !node.directory && !node.placeholder && node.lazy && !node.document;
}

bool SourceTreeModel::nodeEligibleForBackgroundLoad(int nodeIndex) const
{
    if (!nodeNeedsLoad(nodeIndex)) {
        return false;
    }

    const auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
    if (node.section == QStringLiteral("source")
        || node.section == QStringLiteral("summary")
        || node.section == QStringLiteral("signature")) {
        return true;
    }

    return node.section == QStringLiteral("resource")
        && node.contentMode == QStringLiteral("text");
}

std::vector<int> SourceTreeModel::prioritizedPreloadNodeIndices(int centerNode, int maxCount) const
{
    std::vector<int> result;
    if (maxCount <= 0) {
        return result;
    }
    result.reserve(static_cast<std::size_t>(maxCount));

    const auto appendUnique = [this, &result, maxCount](int nodeIndex) {
        if (static_cast<int>(result.size()) >= maxCount
            || !nodeEligibleForBackgroundLoad(nodeIndex)
            || std::ranges::find(result, nodeIndex) != result.end()) {
            return;
        }
        result.push_back(nodeIndex);
    };

    appendUnique(centerNode);

    const int centerRow = rowForNode(centerNode);
    if (centerRow >= 0) {
        constexpr int kVisibleNeighborhoodRows = 80;
        const int firstRow = std::max(0, centerRow - kVisibleNeighborhoodRows / 2);
        const int lastRow = std::min(
            static_cast<int>(visibleRows_.size()) - 1,
            centerRow + kVisibleNeighborhoodRows / 2);
        for (int row = firstRow; row <= lastRow; ++row) {
            appendUnique(visibleRows_.at(static_cast<std::size_t>(row)));
        }
    }

    for (int nodeIndex = 0; nodeIndex < static_cast<int>(nodes_.size()); ++nodeIndex) {
        if (static_cast<int>(result.size()) >= maxCount) {
            break;
        }
        const auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
        if (node.section == QStringLiteral("source")) {
            appendUnique(nodeIndex);
        }
    }

    for (int nodeIndex = 0; nodeIndex < static_cast<int>(nodes_.size()); ++nodeIndex) {
        if (static_cast<int>(result.size()) >= maxCount) {
            break;
        }
        appendUnique(nodeIndex);
    }

    return result;
}

void SourceTreeModel::replaceFiles(std::vector<DecompiledSourceFile> files)
{
    const int previousRow = selectedIndex();
    const int previousNode = selectedNode_;
    beginResetModel();
    rebuildTree(std::move(files));
    selectedNode_ = firstFileNode();
    endResetModel();
    emitSelectedChanged(previousRow, previousNode);
    if (selectedNeedsLoad()) {
        emit fileActivated(selectedNode_);
    }
}

void SourceTreeModel::setNodeContent(int nodeIndex, std::shared_ptr<DocumentContent> document)
{
    if (nodeIndex < 0 || nodeIndex >= static_cast<int>(nodes_.size())) {
        return;
    }
    auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
    node.document = std::move(document);
    if (node.document && !node.document->kind.isEmpty()) {
        node.kind = node.document->kind;
    }
    if (node.document && !node.document->contentMode.isEmpty()) {
        node.contentMode = node.document->contentMode;
    }
    node.lazy = false;

    const int row = rowForNode(nodeIndex);
    if (row >= 0) {
        const auto modelIndex = index(row);
        emit dataChanged(modelIndex, modelIndex, { ContentRole, DiagnosticsRole, KindRole });
    }
    if (selectedNode_ == nodeIndex) {
        emit selectedContentChanged();
        emit diagnosticsChanged();
    }
}

void SourceTreeModel::activateIndex(int index)
{
    if (index < 0 || index >= rowCount()) {
        return;
    }

    const auto nodeIndex = visibleRows_.at(static_cast<std::size_t>(index));
    auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
    if (node.directory) {
        const int previousRow = selectedIndex();
        const int previousNode = selectedNode_;

        if (node.expanded) {
            const int removeCount = visibleDescendantCount(index);
            if (removeCount > 0) {
                beginRemoveRows({}, index + 1, index + removeCount);
                visibleRows_.erase(
                    visibleRows_.begin() + index + 1,
                    visibleRows_.begin() + index + 1 + removeCount);
                node.expanded = false;
                if (selectedNode_ >= 0 && !isNodeVisible(selectedNode_)) {
                    selectedNode_ = nodeIndex;
                }
                endRemoveRows();
            } else {
                node.expanded = false;
                emit dataChanged(this->index(index), this->index(index), { ExpandedRole });
            }
        } else {
            std::vector<int> insertedRows;
            for (int child : node.children) {
                appendVisibleSubtree(child, insertedRows);
            }
            node.expanded = true;
            if (!insertedRows.empty()) {
                beginInsertRows({}, index + 1, index + static_cast<int>(insertedRows.size()));
                visibleRows_.insert(
                    visibleRows_.begin() + index + 1,
                    insertedRows.begin(),
                    insertedRows.end());
                endInsertRows();
            } else {
                emit dataChanged(this->index(index), this->index(index), { ExpandedRole });
            }
        }

        emit dataChanged(this->index(index), this->index(index), { ExpandedRole });
        emitSelectedChanged(previousRow, previousNode);
        return;
    }

    if (!node.placeholder) {
        setSelectedNode(nodeIndex, true);
    }
}

void SourceTreeModel::setSelectedIndex(int index)
{
    if (index < -1) {
        index = -1;
    }
    if (index >= rowCount()) {
        index = rowCount() - 1;
    }
    const int nodeIndex = index >= 0
        ? visibleRows_.at(static_cast<std::size_t>(index))
        : -1;
    setSelectedNode(nodeIndex, true);
}

void SourceTreeModel::rebuildTree(std::vector<DecompiledSourceFile> files)
{
    PerformanceTrace trace(QStringLiteral("SourceTreeModel::rebuildTree"));

    nodes_.clear();
    visibleRows_.clear();

    std::map<QString, int> directories;
    int firstSourceNode = -1;

    const auto addCategory = [this](const QString& name, bool expanded) {
        TreeNode category;
        category.name = name;
        category.path = name;
        category.kind = QStringLiteral("SECTION");
        category.section = name.toLower();
        category.directory = true;
        category.expanded = expanded;
        category.depth = 0;
        category.parent = -1;

        const auto nodeIndex = static_cast<int>(nodes_.size());
        nodes_.push_back(std::move(category));
        return nodeIndex;
    };

    const auto addPlaceholder = [this](int parent, const QString& text) {
        TreeNode placeholder;
        placeholder.name = text;
        placeholder.path = text;
        placeholder.kind = QStringLiteral("PLACEHOLDER");
        placeholder.section = nodes_.at(static_cast<std::size_t>(parent)).section;
        placeholder.placeholder = true;
        placeholder.depth = 1;
        placeholder.parent = parent;

        const auto nodeIndex = static_cast<int>(nodes_.size());
        nodes_.push_back(std::move(placeholder));
        nodes_.at(static_cast<std::size_t>(parent)).children.push_back(nodeIndex);
    };

    const int sourceRoot = addCategory(QStringLiteral("Source code"), true);
    const int resourceRoot = addCategory(QStringLiteral("Resources"), true);

    const auto signatureFile = std::ranges::find_if(files, [](const DecompiledSourceFile& file) {
        return file.section == QStringLiteral("signature");
    });
    const auto summaryFile = std::ranges::find_if(files, [](const DecompiledSourceFile& file) {
        return file.section == QStringLiteral("summary");
    });

    TreeNode signature;
    signature.name = QStringLiteral("APK signature");
    signature.path = QStringLiteral("APK signature");
    signature.kind = QStringLiteral("TXT");
    signature.section = QStringLiteral("signature");
    signature.depth = 0;
    signature.parent = -1;
    if (signatureFile != files.end()) {
        signature.kind = signatureFile->kind;
        signature.contentMode = signatureFile->contentMode;
        if (!signatureFile->content.isEmpty() || !signatureFile->binaryContent.isEmpty() || !signatureFile->lazy) {
            signature.document = makeDocument(std::move(signatureFile->content), std::move(signatureFile->binaryContent), {}, signatureFile->kind, signatureFile->contentMode);
        }
        signature.hyleId = signatureFile->hyleId;
        signature.lazy = signatureFile->lazy;
    } else {
        signature.kind = QStringLiteral("PLACEHOLDER");
        signature.placeholder = true;
        signature.document = makeDocument(QStringLiteral("Waiting for Hyle APK signature API"), {}, {}, signature.kind, QStringLiteral("text"));
    }
    nodes_.push_back(std::move(signature));

    TreeNode summary;
    summary.name = QStringLiteral("Summary");
    summary.path = QStringLiteral("Summary");
    summary.kind = QStringLiteral("TXT");
    summary.section = QStringLiteral("summary");
    summary.depth = 0;
    summary.parent = -1;
    if (summaryFile != files.end()) {
        summary.kind = summaryFile->kind;
        summary.contentMode = summaryFile->contentMode;
        if (!summaryFile->content.isEmpty() || !summaryFile->binaryContent.isEmpty() || !summaryFile->lazy) {
            summary.document = makeDocument(std::move(summaryFile->content), std::move(summaryFile->binaryContent), {}, summaryFile->kind, summaryFile->contentMode);
        }
        summary.hyleId = summaryFile->hyleId;
        summary.lazy = summaryFile->lazy;
    } else {
        summary.kind = QStringLiteral("PLACEHOLDER");
        summary.placeholder = true;
        summary.document = makeDocument(QStringLiteral("Waiting for Hyle summary API"), {}, {}, summary.kind, QStringLiteral("text"));
    }
    nodes_.push_back(std::move(summary));

    const auto addTreeEntry = [this, &directories](int root, DecompiledSourceFile& file, int& firstFileNode) {
        if (file.section != QStringLiteral("source") && file.section != QStringLiteral("resource")) {
            return;
        }

        auto normalizedPath = file.name;
        normalizedPath.replace(QLatin1Char('\\'), QLatin1Char('/'));

        const auto parts = normalizedPath.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        int parent = root;
        QString accumulated = nodes_.at(static_cast<std::size_t>(parent)).path;
        const int directoryPartCount = file.directory ? parts.size() : parts.size() - 1;

        for (int i = 0; i < directoryPartCount; ++i) {
            if (!accumulated.isEmpty()) {
                accumulated += QLatin1Char('/');
            }
            accumulated += parts.at(i);

            const auto directoryKey = file.section + QLatin1Char(':') + accumulated;
            const auto found = directories.find(directoryKey);
            if (found != directories.end()) {
                parent = found->second;
                continue;
            }

            TreeNode directory;
            directory.name = parts.at(i);
            directory.path = accumulated;
            directory.kind = QStringLiteral("DIR");
            directory.section = file.section;
            directory.directory = true;
            directory.expanded = false;
            directory.depth = i + 1;
            directory.parent = parent;

            const auto createdNodeIndex = static_cast<int>(nodes_.size());
            nodes_.push_back(std::move(directory));
            directories.emplace(directoryKey, createdNodeIndex);

            if (parent >= 0) {
                nodes_.at(static_cast<std::size_t>(parent)).children.push_back(createdNodeIndex);
            }
            parent = createdNodeIndex;
        }

        if (file.directory) {
            return;
        }

        TreeNode source;
        source.name = parts.empty() ? normalizedPath : parts.last();
        source.path = normalizedPath;
        source.kind = file.kind;
        source.section = file.section;
        source.contentMode = file.contentMode;
        if (!file.content.isEmpty() || !file.binaryContent.isEmpty() || !file.lazy) {
            source.document = makeDocument(std::move(file.content), std::move(file.binaryContent), {}, file.kind, file.contentMode);
        }
        source.hyleId = file.hyleId;
        source.lazy = file.lazy;
        source.directory = false;
        source.depth = parts.empty() ? 1 : parts.size();
        source.parent = parent;

        const auto createdNodeIndex = static_cast<int>(nodes_.size());
        nodes_.push_back(std::move(source));
        if (firstFileNode < 0) {
            firstFileNode = createdNodeIndex;
        }
        if (parent >= 0) {
            nodes_.at(static_cast<std::size_t>(parent)).children.push_back(createdNodeIndex);
        }
    };

    for (auto& file : files) {
        if (file.section == QStringLiteral("source")) {
            addTreeEntry(sourceRoot, file, firstSourceNode);
        } else if (file.section == QStringLiteral("resource")) {
            int ignoredFirstResourceNode = -1;
            addTreeEntry(resourceRoot, file, ignoredFirstResourceNode);
        }
    }

    if (nodes_.at(static_cast<std::size_t>(resourceRoot)).children.empty()) {
        addPlaceholder(resourceRoot, QStringLiteral("No resources found"));
    }

    for (int nodeIndex = firstSourceNode; nodeIndex >= 0;) {
        auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
        if (node.directory) {
            node.expanded = true;
        }
        nodeIndex = node.parent;
    }

    sortChildren();
    rebuildVisibleRows();
}

void SourceTreeModel::rebuildVisibleRows()
{
    visibleRows_.clear();

    for (int i = 0; i < static_cast<int>(nodes_.size()); ++i) {
        if (nodes_.at(static_cast<std::size_t>(i)).parent < 0) {
            appendVisibleSubtree(i, visibleRows_);
        }
    }
}

void SourceTreeModel::sortChildren()
{
    for (auto& node : nodes_) {
        std::ranges::sort(node.children, [this](int leftIndex, int rightIndex) {
            const auto& left = nodes_.at(static_cast<std::size_t>(leftIndex));
            const auto& right = nodes_.at(static_cast<std::size_t>(rightIndex));
            if (left.directory != right.directory) {
                return left.directory && !right.directory;
            }
            return sortName(left.name) < sortName(right.name);
        });
    }
}

void SourceTreeModel::appendVisibleSubtree(int nodeIndex, std::vector<int>& rows) const
{
    rows.push_back(nodeIndex);
    const auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
    if (!node.directory || !node.expanded) {
        return;
    }
    for (int child : node.children) {
        appendVisibleSubtree(child, rows);
    }
}

int SourceTreeModel::visibleDescendantCount(int row) const
{
    if (row < 0 || row >= static_cast<int>(visibleRows_.size())) {
        return 0;
    }

    const int nodeIndex = visibleRows_.at(static_cast<std::size_t>(row));
    const int depth = nodes_.at(static_cast<std::size_t>(nodeIndex)).depth;
    int count = 0;
    for (int i = row + 1; i < static_cast<int>(visibleRows_.size()); ++i) {
        const auto& candidate = nodes_.at(static_cast<std::size_t>(visibleRows_.at(static_cast<std::size_t>(i))));
        if (candidate.depth <= depth) {
            break;
        }
        ++count;
    }
    return count;
}

int SourceTreeModel::rowForNode(int nodeIndex) const
{
    if (nodeIndex < 0) {
        return -1;
    }
    const auto found = std::ranges::find(visibleRows_, nodeIndex);
    if (found == visibleRows_.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(visibleRows_.begin(), found));
}

int SourceTreeModel::firstFileNode() const
{
    for (int nodeIndex : visibleRows_) {
        const auto& node = nodes_.at(static_cast<std::size_t>(nodeIndex));
        if (!node.directory && !node.placeholder) {
            return nodeIndex;
        }
    }
    return -1;
}

bool SourceTreeModel::isNodeVisible(int nodeIndex) const
{
    return rowForNode(nodeIndex) >= 0;
}

void SourceTreeModel::setSelectedNode(int nodeIndex, bool activateFile)
{
    if (nodeIndex < -1 || nodeIndex >= static_cast<int>(nodes_.size())) {
        nodeIndex = -1;
    }
    const int previousRow = selectedIndex();
    const int previousNode = selectedNode_;
    if (selectedNode_ == nodeIndex) {
        return;
    }
    selectedNode_ = nodeIndex;
    emitSelectedChanged(previousRow, previousNode);
    if (activateFile && selectedNode_ >= 0) {
        const auto& node = nodes_.at(static_cast<std::size_t>(selectedNode_));
        if (node.directory || node.placeholder) {
            return;
        }
        emit fileActivated(selectedNode_);
    }
}

void SourceTreeModel::emitSelectedChanged(int previousRow, int previousNode)
{
    const int currentRow = selectedIndex();
    if (previousRow != currentRow) {
        emit selectedIndexChanged();
    }
    if (previousNode != selectedNode_) {
        emit selectedNameChanged();
        emit selectedContentChanged();
        emit diagnosticsChanged();
        return;
    }
    emit selectedNameChanged();
    emit selectedContentChanged();
    emit diagnosticsChanged();
}
