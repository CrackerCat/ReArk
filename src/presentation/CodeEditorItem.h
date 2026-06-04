#ifndef REARK_CODE_EDITOR_ITEM_H
#define REARK_CODE_EDITOR_ITEM_H

#include "presentation/CodeTheme.h"
#include "presentation/SyntaxHighlighter.h"

#include <QQuickPaintedItem>
#include <QQmlEngine>
#include <QElapsedTimer>
#include <QTextDocument>

class CodeEditorItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
    Q_PROPERTY(QString highlightTheme READ highlightTheme WRITE setHighlightTheme NOTIFY highlightThemeChanged)
    Q_PROPERTY(qreal scrollX READ scrollX WRITE setScrollX NOTIFY scrollXChanged)
    Q_PROPERTY(qreal scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)
    Q_PROPERTY(qreal documentWidth READ documentWidth NOTIFY documentMetricsChanged)
    Q_PROPERTY(qreal documentHeight READ documentHeight NOTIFY documentMetricsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)

public:
    explicit CodeEditorItem(QQuickItem* parent = nullptr);

    [[nodiscard]] QString text() const;
    void setText(const QString& text);
    [[nodiscard]] bool darkTheme() const;
    void setDarkTheme(bool darkTheme);
    [[nodiscard]] QString highlightTheme() const;
    void setHighlightTheme(const QString& highlightTheme);
    [[nodiscard]] qreal scrollX() const;
    void setScrollX(qreal scrollX);
    [[nodiscard]] qreal scrollY() const;
    void setScrollY(qreal scrollY);
    [[nodiscard]] qreal documentWidth() const;
    [[nodiscard]] qreal documentHeight() const;
    [[nodiscard]] bool hasSelection() const;

    Q_INVOKABLE void copySelection() const;
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void selectCurrentLine();
    Q_INVOKABLE void clearSelection();

    void paint(QPainter* painter) override;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

signals:
    void textChanged();
    void darkThemeChanged();
    void highlightThemeChanged();
    void scrollXChanged();
    void scrollYChanged();
    void documentMetricsChanged();
    void selectionChanged();

private:
    void rebuildDocument();
    void applyBaseTextFormat();
    void refreshMetrics();
    void refreshPalette();
    void setCursorPosition(int position, bool keepAnchor);
    void moveCursor(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode);
    void moveCursorByPage(int direction, QTextCursor::MoveMode mode);
    void selectLineAt(int position);
    [[nodiscard]] bool shouldTreatAsTripleClick(const QPointF& point) const;
    [[nodiscard]] int visibleLineCount() const;
    void notifySelectionChanged(bool previousHasSelection);
    [[nodiscard]] QString normalizedText(const QString& text) const;
    [[nodiscard]] int positionAt(const QPointF& point) const;
    [[nodiscard]] QTextCursor selectionCursor() const;
    [[nodiscard]] int lineCount() const;
    [[nodiscard]] qreal gutterWidth() const;

    QTextDocument document_;
    SyntaxHighlighter highlighter_;
    QString text_;
    bool darkTheme_ = true;
    QString highlightTheme_ = QStringLiteral("github-dark");
    qreal scrollX_ = 0.0;
    qreal scrollY_ = 0.0;
    qreal documentWidth_ = 0.0;
    qreal documentHeight_ = 0.0;
    int selectionAnchor_ = -1;
    int selectionPosition_ = -1;
    bool selecting_ = false;
    QElapsedTimer doubleClickTimer_;
    QPointF lastDoubleClickPoint_;
    int lastDoubleClickBlock_ = -1;
    QColor editorColor_;
    QColor textColor_;
    QColor gutterColor_;
    QColor gutterTextColor_;
    QColor dividerColor_;
    QColor currentLineColor_;
    QColor selectionColor_;
    QColor selectedTextColor_;
};

#endif // REARK_CODE_EDITOR_ITEM_H
