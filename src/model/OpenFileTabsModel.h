#ifndef REARK_OPEN_FILE_TABS_MODEL_H
#define REARK_OPEN_FILE_TABS_MODEL_H

#include <QAbstractListModel>
#include <QString>

#include <vector>

class OpenFileTabsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int activeIndex READ activeIndex WRITE setActiveIndex NOTIFY activeIndexChanged)
    Q_PROPERTY(QString activeContent READ activeContent NOTIFY activeTabChanged)
    Q_PROPERTY(QString activeContentMode READ activeContentMode NOTIFY activeTabChanged)
    Q_PROPERTY(QString activeName READ activeName NOTIFY activeTabChanged)
    Q_PROPERTY(QString activePath READ activePath NOTIFY activeTabChanged)
    Q_PROPERTY(QString activeDiagnostics READ activeDiagnostics NOTIFY activeTabChanged)
    Q_PROPERTY(bool hasTabs READ hasTabs NOTIFY tabsChanged)

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        PathRole,
        ContentRole,
        ContentModeRole,
        DiagnosticsRole,
        NodeIndexRole,
        LoadingRole,
        ActiveRole
    };

    explicit OpenFileTabsModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] int activeIndex() const;
    [[nodiscard]] QString activeContent() const;
    [[nodiscard]] QString activeContentMode() const;
    [[nodiscard]] QString activeName() const;
    [[nodiscard]] QString activePath() const;
    [[nodiscard]] QString activeDiagnostics() const;
    [[nodiscard]] bool hasTabs() const;

    void clear();
    void openOrActivate(int nodeIndex, const QString& name, const QString& path, const QString& content, const QString& contentMode, const QString& diagnostics, bool loading);
    void updateNode(int nodeIndex, const QString& content, const QString& contentMode, const QString& diagnostics);
    void setNodeLoading(int nodeIndex, bool loading);

    Q_INVOKABLE void closeTab(int index);

public slots:
    void setActiveIndex(int index);

signals:
    void activeIndexChanged();
    void activeTabChanged();
    void tabsChanged();

private:
    struct Tab {
        int nodeIndex = -1;
        QString name;
        QString path;
        QString content;
        QString contentMode = QStringLiteral("text");
        QString diagnostics;
        bool loading = false;
    };

    [[nodiscard]] int tabIndexForNode(int nodeIndex) const;
    void emitActiveChanged();

    std::vector<Tab> tabs_;
    int activeIndex_ = -1;
};

#endif // REARK_OPEN_FILE_TABS_MODEL_H
