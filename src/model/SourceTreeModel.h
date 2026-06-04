#ifndef REARK_SOURCE_TREE_MODEL_H
#define REARK_SOURCE_TREE_MODEL_H

#include <QAbstractListModel>
#include <QString>

#include <cstddef>
#include <vector>

struct DecompiledSourceFile {
    QString name;
    QString kind;
    QString content;
    QString section;
    QString contentMode = QStringLiteral("text");
    std::size_t hyleId = 0;
    bool lazy = false;
    bool directory = false;
};

class SourceTreeModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString selectedContent READ selectedContent NOTIFY selectedContentChanged)
    Q_PROPERTY(QString selectedName READ selectedName NOTIFY selectedNameChanged)
    Q_PROPERTY(QString diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        PathRole,
        KindRole,
        ContentRole,
        DiagnosticsRole,
        DepthRole,
        DirectoryRole,
        ExpandedRole,
        PlaceholderRole
    };

    explicit SourceTreeModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString selectedContent() const;
    [[nodiscard]] QString selectedName() const;
    [[nodiscard]] QString diagnostics() const;
    [[nodiscard]] int selectedIndex() const;
    [[nodiscard]] int selectedNode() const;
    [[nodiscard]] bool selectedNeedsLoad() const;
    [[nodiscard]] std::size_t selectedHyleId() const;
    [[nodiscard]] QString selectedFileName() const;
    [[nodiscard]] QString nodeContent(int nodeIndex) const;
    [[nodiscard]] QString nodeDiagnostics(int nodeIndex) const;
    [[nodiscard]] std::size_t nodeHyleId(int nodeIndex) const;
    [[nodiscard]] QString nodeName(int nodeIndex) const;
    [[nodiscard]] QString nodePath(int nodeIndex) const;
    [[nodiscard]] QString nodeSection(int nodeIndex) const;
    [[nodiscard]] QString nodeContentMode(int nodeIndex) const;
    [[nodiscard]] bool nodeNeedsLoad(int nodeIndex) const;

    void replaceFiles(std::vector<DecompiledSourceFile> files);
    void setNodeContent(int nodeIndex, const QString& content, const QString& diagnostics, const QString& kind, const QString& contentMode);
    void markNodeFailed(int nodeIndex, const QString& error);
    Q_INVOKABLE void activateIndex(int index);

public slots:
    void setSelectedIndex(int index);

signals:
    void selectedContentChanged();
    void selectedNameChanged();
    void diagnosticsChanged();
    void selectedIndexChanged();
    void fileActivated(int nodeIndex);

private:
    struct TreeNode {
        QString name;
        QString path;
        QString kind;
        QString content;
        QString diagnostics;
        QString section;
        QString contentMode = QStringLiteral("text");
        std::size_t hyleId = 0;
        bool lazy = false;
        bool directory = false;
        bool expanded = true;
        bool placeholder = false;
        int depth = 0;
        int parent = -1;
        std::vector<int> children;
    };

    void rebuildTree(std::vector<DecompiledSourceFile> files);
    void rebuildVisibleRows();
    [[nodiscard]] int rowForNode(int nodeIndex) const;
    [[nodiscard]] int firstFileNode() const;
    [[nodiscard]] bool isNodeVisible(int nodeIndex) const;
    void setSelectedNode(int nodeIndex, bool activateFile);
    void emitSelectedChanged(int previousRow, int previousNode);

    std::vector<TreeNode> nodes_;
    std::vector<int> visibleRows_;
    int selectedNode_ = -1;
};

#endif // REARK_SOURCE_TREE_MODEL_H
