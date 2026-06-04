#include "model/OpenFileTabsModel.h"

#include <algorithm>

OpenFileTabsModel::OpenFileTabsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int OpenFileTabsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(tabs_.size());
}

QVariant OpenFileTabsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) {
        return {};
    }

    const auto& tab = tabs_.at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return tab.name;
    case PathRole:
        return tab.path;
    case ContentRole:
        return tab.content;
    case ContentModeRole:
        return tab.contentMode;
    case DiagnosticsRole:
        return tab.diagnostics;
    case NodeIndexRole:
        return tab.nodeIndex;
    case LoadingRole:
        return tab.loading;
    case ActiveRole:
        return index.row() == activeIndex_;
    default:
        return {};
    }
}

QHash<int, QByteArray> OpenFileTabsModel::roleNames() const
{
    return {
        { NameRole, "name" },
        { PathRole, "path" },
        { ContentRole, "content" },
        { ContentModeRole, "contentMode" },
        { DiagnosticsRole, "diagnostics" },
        { NodeIndexRole, "nodeIndex" },
        { LoadingRole, "loading" },
        { ActiveRole, "active" }
    };
}

int OpenFileTabsModel::activeIndex() const
{
    return activeIndex_;
}

QString OpenFileTabsModel::activeContent() const
{
    if (activeIndex_ < 0 || activeIndex_ >= rowCount()) {
        return tr("// Drop a package to start decompiling.");
    }
    const auto& tab = tabs_.at(static_cast<std::size_t>(activeIndex_));
    if (tab.loading && tab.content.isEmpty()) {
        return tr("// Decompiling selected source file...");
    }
    return tab.content;
}

QString OpenFileTabsModel::activeContentMode() const
{
    if (activeIndex_ < 0 || activeIndex_ >= rowCount()) {
        return QStringLiteral("text");
    }
    return tabs_.at(static_cast<std::size_t>(activeIndex_)).contentMode;
}

QString OpenFileTabsModel::activeName() const
{
    if (activeIndex_ < 0 || activeIndex_ >= rowCount()) {
        return {};
    }
    return tabs_.at(static_cast<std::size_t>(activeIndex_)).name;
}

QString OpenFileTabsModel::activePath() const
{
    if (activeIndex_ < 0 || activeIndex_ >= rowCount()) {
        return {};
    }
    return tabs_.at(static_cast<std::size_t>(activeIndex_)).path;
}

QString OpenFileTabsModel::activeDiagnostics() const
{
    if (activeIndex_ < 0 || activeIndex_ >= rowCount()) {
        return {};
    }
    return tabs_.at(static_cast<std::size_t>(activeIndex_)).diagnostics;
}

bool OpenFileTabsModel::hasTabs() const
{
    return !tabs_.empty();
}

void OpenFileTabsModel::clear()
{
    if (tabs_.empty()) {
        return;
    }

    beginResetModel();
    tabs_.clear();
    activeIndex_ = -1;
    endResetModel();
    emit activeIndexChanged();
    emitActiveChanged();
    emit tabsChanged();
}

void OpenFileTabsModel::openOrActivate(int nodeIndex, const QString& name, const QString& path, const QString& content, const QString& contentMode, const QString& diagnostics, bool loading)
{
    const int existingIndex = tabIndexForNode(nodeIndex);
    if (existingIndex >= 0) {
        auto& tab = tabs_.at(static_cast<std::size_t>(existingIndex));
        tab.name = name;
        tab.path = path;
        tab.content = content;
        tab.contentMode = contentMode;
        tab.diagnostics = diagnostics;
        tab.loading = loading;
        const auto modelIndex = index(existingIndex);
        emit dataChanged(modelIndex, modelIndex, { NameRole, PathRole, ContentRole, ContentModeRole, DiagnosticsRole, LoadingRole });
        setActiveIndex(existingIndex);
        return;
    }

    const int insertIndex = rowCount();
    beginInsertRows({}, insertIndex, insertIndex);
    tabs_.push_back({
        nodeIndex,
        name,
        path,
        content,
        contentMode,
        diagnostics,
        loading
    });
    endInsertRows();
    emit tabsChanged();
    setActiveIndex(insertIndex);
}

void OpenFileTabsModel::updateNode(int nodeIndex, const QString& content, const QString& contentMode, const QString& diagnostics)
{
    const int tabIndex = tabIndexForNode(nodeIndex);
    if (tabIndex < 0) {
        return;
    }

    auto& tab = tabs_.at(static_cast<std::size_t>(tabIndex));
    tab.content = content;
    if (!contentMode.isEmpty()) {
        tab.contentMode = contentMode;
    }
    tab.diagnostics = diagnostics;
    tab.loading = false;

    const auto modelIndex = index(tabIndex);
    emit dataChanged(modelIndex, modelIndex, { ContentRole, ContentModeRole, DiagnosticsRole, LoadingRole });
    if (tabIndex == activeIndex_) {
        emitActiveChanged();
    }
}

void OpenFileTabsModel::setNodeLoading(int nodeIndex, bool loading)
{
    const int tabIndex = tabIndexForNode(nodeIndex);
    if (tabIndex < 0) {
        return;
    }

    auto& tab = tabs_.at(static_cast<std::size_t>(tabIndex));
    if (tab.loading == loading) {
        return;
    }
    tab.loading = loading;
    const auto modelIndex = index(tabIndex);
    emit dataChanged(modelIndex, modelIndex, { LoadingRole });
    if (tabIndex == activeIndex_) {
        emitActiveChanged();
    }
}

void OpenFileTabsModel::closeTab(int indexToClose)
{
    if (indexToClose < 0 || indexToClose >= rowCount()) {
        return;
    }

    const bool closingActive = indexToClose == activeIndex_;
    beginRemoveRows({}, indexToClose, indexToClose);
    tabs_.erase(tabs_.begin() + indexToClose);
    endRemoveRows();

    if (tabs_.empty()) {
        activeIndex_ = -1;
    } else if (closingActive) {
        activeIndex_ = std::min(indexToClose, rowCount() - 1);
    } else if (indexToClose < activeIndex_) {
        --activeIndex_;
    }

    emit tabsChanged();
    emit activeIndexChanged();
    emitActiveChanged();
}

void OpenFileTabsModel::setActiveIndex(int index)
{
    if (index < -1) {
        index = -1;
    }
    if (index >= rowCount()) {
        index = rowCount() - 1;
    }
    if (activeIndex_ == index) {
        return;
    }

    const int previousIndex = activeIndex_;
    activeIndex_ = index;
    if (previousIndex >= 0) {
        const auto modelIndex = this->index(previousIndex);
        emit dataChanged(modelIndex, modelIndex, { ActiveRole });
    }
    if (activeIndex_ >= 0) {
        const auto modelIndex = this->index(activeIndex_);
        emit dataChanged(modelIndex, modelIndex, { ActiveRole });
    }
    emit activeIndexChanged();
    emitActiveChanged();
}

int OpenFileTabsModel::tabIndexForNode(int nodeIndex) const
{
    const auto found = std::ranges::find_if(tabs_, [nodeIndex](const Tab& tab) {
        return tab.nodeIndex == nodeIndex;
    });
    if (found == tabs_.end()) {
        return -1;
    }
    return static_cast<int>(std::distance(tabs_.begin(), found));
}

void OpenFileTabsModel::emitActiveChanged()
{
    emit activeTabChanged();
}
