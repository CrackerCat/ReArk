#include "presentation/CodeEditorItem.h"

#include <QAbstractTextDocumentLayout>
#include <QGuiApplication>
#include <QFont>
#include <QFontMetricsF>
#include <QClipboard>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleHints>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextOption>

#include <algorithm>
#include <cmath>

namespace {

constexpr qreal kFontPixelSize = 14.0;
constexpr qreal kTopPadding = 12.0;
constexpr qreal kBottomPadding = 12.0;
constexpr qreal kLeftPadding = 14.0;
constexpr qreal kRightPadding = 14.0;
constexpr qreal kGutterPadding = 10.0;
constexpr qreal kMinimumGutterWidth = 48.0;

QFont editorFont()
{
    QFont font(QStringLiteral("Consolas"));
    font.setPixelSize(static_cast<int>(kFontPixelSize));
    return font;
}

} // namespace

CodeEditorItem::CodeEditorItem(QQuickItem* parent)
    : QQuickPaintedItem(parent),
      highlighter_(&document_)
{
    setAntialiasing(false);
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QQuickItem::ItemIsFocusScope, true);

    document_.setDocumentMargin(0.0);
    document_.setDefaultFont(editorFont());

    QTextOption option;
    option.setWrapMode(QTextOption::NoWrap);
    option.setFlags(QTextOption::IncludeTrailingSpaces);
    document_.setDefaultTextOption(option);

    refreshPalette();
    rebuildDocument();
}

QString CodeEditorItem::text() const
{
    return text_;
}

void CodeEditorItem::setText(const QString& text)
{
    const QString normalized = normalizedText(text);
    if (text_ == normalized) {
        return;
    }

    text_ = normalized;
    rebuildDocument();
    emit textChanged();
}

bool CodeEditorItem::darkTheme() const
{
    return darkTheme_;
}

void CodeEditorItem::setDarkTheme(bool darkTheme)
{
    if (darkTheme_ == darkTheme) {
        return;
    }

    darkTheme_ = darkTheme;
    refreshPalette();
    applyBaseTextFormat();
    highlighter_.setTheme(highlightTheme_, darkTheme_);
    update();
    emit darkThemeChanged();
}

QString CodeEditorItem::highlightTheme() const
{
    return highlightTheme_;
}

void CodeEditorItem::setHighlightTheme(const QString& highlightTheme)
{
    if (highlightTheme_ == highlightTheme) {
        return;
    }

    highlightTheme_ = highlightTheme;
    refreshPalette();
    applyBaseTextFormat();
    highlighter_.setTheme(highlightTheme_, darkTheme_);
    refreshMetrics();
    update();
    emit highlightThemeChanged();
}

qreal CodeEditorItem::scrollX() const
{
    return scrollX_;
}

void CodeEditorItem::setScrollX(qreal scrollX)
{
    scrollX = std::max<qreal>(0.0, scrollX);
    if (qFuzzyCompare(scrollX_ + 1.0, scrollX + 1.0)) {
        return;
    }

    scrollX_ = scrollX;
    update();
    emit scrollXChanged();
}

qreal CodeEditorItem::scrollY() const
{
    return scrollY_;
}

void CodeEditorItem::setScrollY(qreal scrollY)
{
    scrollY = std::max<qreal>(0.0, scrollY);
    if (qFuzzyCompare(scrollY_ + 1.0, scrollY + 1.0)) {
        return;
    }

    scrollY_ = scrollY;
    update();
    emit scrollYChanged();
}

qreal CodeEditorItem::documentWidth() const
{
    return documentWidth_;
}

qreal CodeEditorItem::documentHeight() const
{
    return documentHeight_;
}

bool CodeEditorItem::hasSelection() const
{
    return selectionCursor().hasSelection();
}

void CodeEditorItem::copySelection() const
{
    QTextCursor cursor = selectionCursor();
    if (!cursor.hasSelection()) {
        return;
    }
    QGuiApplication::clipboard()->setText(cursor.selectedText().replace(QChar::ParagraphSeparator, QLatin1Char('\n')));
}

void CodeEditorItem::selectAll()
{
    const bool previousHasSelection = hasSelection();
    selectionAnchor_ = 0;
    selectionPosition_ = std::max(0, document_.characterCount() - 1);
    update();
    notifySelectionChanged(previousHasSelection);
}

void CodeEditorItem::selectCurrentLine()
{
    const bool previousHasSelection = hasSelection();
    const int position = selectionPosition_ >= 0 ? selectionPosition_ : 0;
    selectLineAt(position);
    update();
    notifySelectionChanged(previousHasSelection);
}

void CodeEditorItem::clearSelection()
{
    const bool previousHasSelection = hasSelection();
    if (selectionPosition_ < 0) {
        selectionPosition_ = 0;
    }
    selectionAnchor_ = selectionPosition_;
    update();
    notifySelectionChanged(previousHasSelection);
}

void CodeEditorItem::paint(QPainter* painter)
{
    painter->fillRect(boundingRect(), editorColor_);

    const qreal gutter = gutterWidth();
    painter->fillRect(QRectF(0.0, 0.0, gutter, height()), gutterColor_);
    painter->fillRect(QRectF(gutter - 1.0, 0.0, 1.0, height()), dividerColor_);

    painter->setFont(editorFont());

    const auto* layout = document_.documentLayout();
    const qreal codeTop = kTopPadding - scrollY_;
    const int currentBlockNumber = selectionPosition_ >= 0
        ? document_.findBlock(selectionPosition_).blockNumber()
        : -1;

    painter->setPen(gutterTextColor_);
    for (QTextBlock block = document_.firstBlock(); block.isValid(); block = block.next()) {
        const QRectF blockRect = layout->blockBoundingRect(block);
        const qreal y = codeTop + blockRect.top();
        const qreal blockHeight = blockRect.height();
        if (y > height()) {
            break;
        }
        if (y + blockHeight < 0.0) {
            continue;
        }

        if (block.blockNumber() == currentBlockNumber && hasActiveFocus()) {
            painter->fillRect(QRectF(gutter, y, width() - gutter, blockHeight), currentLineColor_);
        }

        const QRectF numberRect(0.0, y, gutter - kGutterPadding, blockHeight);
        painter->drawText(numberRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(block.blockNumber() + 1));
    }

    painter->save();
    painter->setClipRect(QRectF(gutter, 0.0, width() - gutter, height()));
    painter->translate(gutter + kLeftPadding - scrollX_, kTopPadding - scrollY_);

    QAbstractTextDocumentLayout::PaintContext context;
    context.clip = QRectF(scrollX_, scrollY_, width() - gutter, height());
    context.palette.setColor(QPalette::Text, textColor_);

    QTextCursor cursor = selectionCursor();
    if (cursor.hasSelection()) {
        QAbstractTextDocumentLayout::Selection selection;
        selection.cursor = cursor;
        selection.format.setBackground(selectionColor_);
        selection.format.setForeground(selectedTextColor_);
        context.selections.push_back(selection);
    }

    document_.documentLayout()->draw(painter, context);
    painter->restore();
}

void CodeEditorItem::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    forceActiveFocus();
    const bool previousHasSelection = hasSelection();

    if (shouldTreatAsTripleClick(event->position())) {
        selectLineAt(positionAt(event->position()));
        selecting_ = false;
        doubleClickTimer_.invalidate();
        update();
        notifySelectionChanged(previousHasSelection);
        event->accept();
        return;
    }

    setCursorPosition(positionAt(event->position()), false);
    selecting_ = true;
    update();
    notifySelectionChanged(previousHasSelection);
    event->accept();
}

void CodeEditorItem::mouseMoveEvent(QMouseEvent* event)
{
    if (!selecting_) {
        event->ignore();
        return;
    }

    const bool previousHasSelection = hasSelection();
    setCursorPosition(positionAt(event->position()), true);
    update();
    notifySelectionChanged(previousHasSelection);
    event->accept();
}

void CodeEditorItem::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    if (!selecting_) {
        event->accept();
        return;
    }

    selecting_ = false;
    const bool previousHasSelection = hasSelection();
    setCursorPosition(positionAt(event->position()), true);
    update();
    notifySelectionChanged(previousHasSelection);
    event->accept();
}

void CodeEditorItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }

    forceActiveFocus();
    const bool previousHasSelection = hasSelection();
    QTextCursor cursor(&document_);
    cursor.setPosition(positionAt(event->position()));
    cursor.select(QTextCursor::WordUnderCursor);
    selectionAnchor_ = cursor.anchor();
    selectionPosition_ = cursor.position();
    selecting_ = false;
    lastDoubleClickPoint_ = event->position();
    lastDoubleClickBlock_ = document_.findBlock(selectionPosition_).blockNumber();
    doubleClickTimer_.restart();
    update();
    notifySelectionChanged(previousHasSelection);
    event->accept();
}

void CodeEditorItem::keyPressEvent(QKeyEvent* event)
{
    if (event->matches(QKeySequence::Copy)) {
        copySelection();
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::SelectAll)) {
        selectAll();
        event->accept();
        return;
    }

    const QTextCursor::MoveMode mode = event->modifiers().testFlag(Qt::ShiftModifier)
        ? QTextCursor::KeepAnchor
        : QTextCursor::MoveAnchor;
    const bool control = event->modifiers().testFlag(Qt::ControlModifier);

    switch (event->key()) {
    case Qt::Key_Left:
        moveCursor(control ? QTextCursor::PreviousWord : QTextCursor::Left, mode);
        event->accept();
        return;
    case Qt::Key_Right:
        moveCursor(control ? QTextCursor::NextWord : QTextCursor::Right, mode);
        event->accept();
        return;
    case Qt::Key_Up:
        moveCursor(QTextCursor::Up, mode);
        event->accept();
        return;
    case Qt::Key_Down:
        moveCursor(QTextCursor::Down, mode);
        event->accept();
        return;
    case Qt::Key_Home:
        moveCursor(control ? QTextCursor::Start : QTextCursor::StartOfLine, mode);
        event->accept();
        return;
    case Qt::Key_End:
        moveCursor(control ? QTextCursor::End : QTextCursor::EndOfLine, mode);
        event->accept();
        return;
    case Qt::Key_PageUp:
        moveCursorByPage(-1, mode);
        event->accept();
        return;
    case Qt::Key_PageDown:
        moveCursorByPage(1, mode);
        event->accept();
        return;
    case Qt::Key_Escape:
        clearSelection();
        event->accept();
        return;
    default:
        break;
    }

    QQuickPaintedItem::keyPressEvent(event);
}

void CodeEditorItem::rebuildDocument()
{
    document_.setDefaultFont(editorFont());
    document_.setPlainText(text_);
    selectionAnchor_ = -1;
    selectionPosition_ = -1;
    selecting_ = false;
    applyBaseTextFormat();
    highlighter_.rehighlight();
    refreshMetrics();
    update();
}

void CodeEditorItem::applyBaseTextFormat()
{
    QTextCursor cursor(&document_);
    cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    format.setForeground(textColor_);
    format.setFont(editorFont());
    cursor.mergeCharFormat(format);
}

void CodeEditorItem::refreshMetrics()
{
    const qreal oldWidth = documentWidth_;
    const qreal oldHeight = documentHeight_;
    documentWidth_ = gutterWidth() + kLeftPadding + document_.idealWidth() + kRightPadding;
    documentHeight_ = kTopPadding + document_.size().height() + kBottomPadding;

    if (!qFuzzyCompare(oldWidth + 1.0, documentWidth_ + 1.0)
        || !qFuzzyCompare(oldHeight + 1.0, documentHeight_ + 1.0)) {
        emit documentMetricsChanged();
    }
}

void CodeEditorItem::refreshPalette()
{
    const CodeTheme theme = codeThemeForId(highlightTheme_, darkTheme_);
    editorColor_ = theme.editor;
    textColor_ = theme.text;
    gutterColor_ = theme.gutter;
    gutterTextColor_ = theme.gutterText;
    dividerColor_ = theme.divider;
    currentLineColor_ = theme.currentLine;
    selectionColor_ = theme.selection;
    selectedTextColor_ = theme.selectedText;
}

void CodeEditorItem::setCursorPosition(int position, bool keepAnchor)
{
    position = std::clamp(position, 0, std::max(0, document_.characterCount() - 1));
    if (!keepAnchor || selectionAnchor_ < 0) {
        selectionAnchor_ = position;
    }
    selectionPosition_ = position;
}

void CodeEditorItem::moveCursor(QTextCursor::MoveOperation operation, QTextCursor::MoveMode mode)
{
    const bool previousHasSelection = hasSelection();
    QTextCursor cursor(&document_);
    const int position = selectionPosition_ >= 0 ? selectionPosition_ : 0;
    cursor.setPosition(std::clamp(position, 0, std::max(0, document_.characterCount() - 1)));
    if (mode == QTextCursor::KeepAnchor && selectionAnchor_ >= 0) {
        cursor.setPosition(selectionAnchor_);
        cursor.setPosition(position, QTextCursor::KeepAnchor);
    }
    cursor.movePosition(operation, mode);
    selectionAnchor_ = cursor.anchor();
    selectionPosition_ = cursor.position();
    update();
    notifySelectionChanged(previousHasSelection);
}

void CodeEditorItem::moveCursorByPage(int direction, QTextCursor::MoveMode mode)
{
    const int steps = visibleLineCount();
    const auto operation = direction < 0 ? QTextCursor::Up : QTextCursor::Down;
    const bool previousHasSelection = hasSelection();
    QTextCursor cursor(&document_);
    const int position = selectionPosition_ >= 0 ? selectionPosition_ : 0;
    cursor.setPosition(std::clamp(position, 0, std::max(0, document_.characterCount() - 1)));
    if (mode == QTextCursor::KeepAnchor && selectionAnchor_ >= 0) {
        cursor.setPosition(selectionAnchor_);
        cursor.setPosition(position, QTextCursor::KeepAnchor);
    }

    for (int i = 0; i < steps; ++i) {
        if (!cursor.movePosition(operation, mode)) {
            break;
        }
    }

    selectionAnchor_ = cursor.anchor();
    selectionPosition_ = cursor.position();
    update();
    notifySelectionChanged(previousHasSelection);
}

void CodeEditorItem::selectLineAt(int position)
{
    QTextCursor cursor(&document_);
    cursor.setPosition(std::clamp(position, 0, std::max(0, document_.characterCount() - 1)));
    cursor.select(QTextCursor::LineUnderCursor);
    selectionAnchor_ = cursor.anchor();
    selectionPosition_ = cursor.position();
}

bool CodeEditorItem::shouldTreatAsTripleClick(const QPointF& point) const
{
    if (!doubleClickTimer_.isValid()) {
        return false;
    }
    const int interval = QGuiApplication::styleHints() != nullptr
        ? QGuiApplication::styleHints()->mouseDoubleClickInterval()
        : 400;
    if (doubleClickTimer_.elapsed() > interval) {
        return false;
    }
    if (std::abs(point.x() - lastDoubleClickPoint_.x()) > 4.0
        || std::abs(point.y() - lastDoubleClickPoint_.y()) > 4.0) {
        return false;
    }
    const int block = document_.findBlock(positionAt(point)).blockNumber();
    return block >= 0 && block == lastDoubleClickBlock_;
}

int CodeEditorItem::visibleLineCount() const
{
    const QFontMetricsF metrics(editorFont());
    const qreal lineHeight = std::max<qreal>(1.0, metrics.lineSpacing());
    return std::max(1, static_cast<int>((height() - kTopPadding - kBottomPadding) / lineHeight));
}

void CodeEditorItem::notifySelectionChanged(bool previousHasSelection)
{
    if (previousHasSelection != hasSelection()) {
        emit selectionChanged();
    }
}

QString CodeEditorItem::normalizedText(const QString& text) const
{
    QString normalized = text;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    QStringList lines = normalized.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }
    return lines.join(QLatin1Char('\n'));
}

int CodeEditorItem::positionAt(const QPointF& point) const
{
    const qreal gutter = gutterWidth();
    const QPointF documentPoint(
        point.x() - gutter - kLeftPadding + scrollX_,
        point.y() - kTopPadding + scrollY_);
    const int position = document_.documentLayout()->hitTest(documentPoint, Qt::FuzzyHit);
    return std::clamp(position, 0, std::max(0, document_.characterCount() - 1));
}

QTextCursor CodeEditorItem::selectionCursor() const
{
    QTextCursor cursor(const_cast<QTextDocument*>(&document_));
    if (selectionAnchor_ < 0 || selectionPosition_ < 0 || selectionAnchor_ == selectionPosition_) {
        return cursor;
    }

    cursor.setPosition(selectionAnchor_);
    cursor.setPosition(selectionPosition_, QTextCursor::KeepAnchor);
    return cursor;
}

int CodeEditorItem::lineCount() const
{
    return std::max(1, document_.blockCount());
}

qreal CodeEditorItem::gutterWidth() const
{
    const int digits = QString::number(lineCount()).length();
    const QFontMetricsF metrics(editorFont());
    return std::max(kMinimumGutterWidth, kGutterPadding * 2.0 + metrics.horizontalAdvance(QString(digits, QLatin1Char('9'))));
}
