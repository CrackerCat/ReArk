#ifndef REARK_CODE_EDITOR_ITEM_H
#define REARK_CODE_EDITOR_ITEM_H

#include "presentation/CodeTheme.h"

#include <QQuickPaintedItem>
#include <QQmlEngine>
#include <QElapsedTimer>
#include <QStringView>
#include <QVector>

#include <memory>

class CodeLineHighlighter;

class CodeEditorItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(bool darkTheme READ darkTheme WRITE setDarkTheme NOTIFY darkThemeChanged)
    Q_PROPERTY(QString highlightTheme READ highlightTheme WRITE setHighlightTheme NOTIFY highlightThemeChanged)
    Q_PROPERTY(QString syntax READ syntax WRITE setSyntax NOTIFY syntaxChanged)
    Q_PROPERTY(bool fastScrolling READ fastScrolling WRITE setFastScrolling NOTIFY fastScrollingChanged)
    Q_PROPERTY(qreal scrollX READ scrollX WRITE setScrollX NOTIFY scrollXChanged)
    Q_PROPERTY(qreal scrollY READ scrollY WRITE setScrollY NOTIFY scrollYChanged)
    Q_PROPERTY(qreal documentWidth READ documentWidth NOTIFY documentMetricsChanged)
    Q_PROPERTY(qreal documentHeight READ documentHeight NOTIFY documentMetricsChanged)
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY selectionChanged)

public:
    explicit CodeEditorItem(QQuickItem* parent = nullptr);
    ~CodeEditorItem() override;

    [[nodiscard]] QString text() const;
    void setText(const QString& text);
    [[nodiscard]] bool darkTheme() const;
    void setDarkTheme(bool darkTheme);
    [[nodiscard]] QString highlightTheme() const;
    void setHighlightTheme(const QString& highlightTheme);
    [[nodiscard]] QString syntax() const;
    void setSyntax(const QString& syntax);
    [[nodiscard]] bool fastScrolling() const;
    void setFastScrolling(bool fastScrolling);
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
    void syntaxChanged();
    void fastScrollingChanged();
    void scrollXChanged();
    void scrollYChanged();
    void documentMetricsChanged();
    void selectionChanged();

private:
    enum class CursorMove {
        Left,
        Right,
        Up,
        Down,
        LineStart,
        LineEnd,
        DocumentStart,
        DocumentEnd,
        PreviousWord,
        NextWord
    };

    void rebuildDocument();
    void rebuildLineIndex();
    void refreshSyntaxHighlighter();
    void refreshMetrics();
    void refreshPalette();
    void refreshTextMetrics();
    void setCursorPosition(int position, bool keepAnchor);
    void moveCursor(CursorMove operation, bool keepAnchor);
    void moveCursorByPage(int direction, bool keepAnchor);
    void selectLineAt(int position);
    [[nodiscard]] bool shouldTreatAsTripleClick(const QPointF& point) const;
    [[nodiscard]] int visibleLineCount() const;
    void notifySelectionChanged(bool previousHasSelection);
    [[nodiscard]] QString normalizedText(const QString& text) const;
    [[nodiscard]] int positionAt(const QPointF& point) const;
    [[nodiscard]] int lineCount() const;
    [[nodiscard]] qreal gutterWidth() const;
    [[nodiscard]] int lineForPosition(int position) const;
    [[nodiscard]] int lineStartPosition(int line) const;
    [[nodiscard]] int lineEndPosition(int line) const;
    [[nodiscard]] QStringView lineView(int line) const;
    [[nodiscard]] int visualColumnForPosition(int line, int position) const;
    [[nodiscard]] int positionForVisualColumn(int line, int visualColumn) const;
    [[nodiscard]] int movedPosition(int position, CursorMove operation) const;
    [[nodiscard]] int previousWordPosition(int position) const;
    [[nodiscard]] int nextWordPosition(int position) const;
    [[nodiscard]] static bool isWordCharacter(QChar ch);
    [[nodiscard]] static QString expandedTabs(QStringView text);

    std::unique_ptr<CodeLineHighlighter> syntaxHighlighter_;
    QString text_;
    QVector<int> lineStarts_;
    bool darkTheme_ = true;
    bool fastScrolling_ = false;
    QString highlightTheme_ = QStringLiteral("GitHub Dark");
    QString syntax_;
    qreal lineHeight_ = 1.0;
    qreal lineAscent_ = 1.0;
    qreal characterWidth_ = 1.0;
    int longestLineColumns_ = 0;
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
