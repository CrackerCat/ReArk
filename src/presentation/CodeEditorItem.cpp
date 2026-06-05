#include "presentation/CodeEditorItem.h"

#include "core/PerformanceTrace.h"

#include <KSyntaxHighlighting/AbstractHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/State>
#include <KSyntaxHighlighting/Theme>

#include <QClipboard>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleHints>

#include <algorithm>
#include <cmath>
#include <functional>

namespace {

constexpr qreal kFontPixelSize = 14.0;
constexpr qreal kTopPadding = 12.0;
constexpr qreal kBottomPadding = 12.0;
constexpr qreal kLeftPadding = 14.0;
constexpr qreal kRightPadding = 14.0;
constexpr qreal kGutterPadding = 10.0;
constexpr qreal kMinimumGutterWidth = 48.0;
constexpr int kTabColumns = 4;
constexpr qsizetype kHighlightCharacterLimit = 1'500'000;
constexpr int kHighlightLineLimit = 30'000;
constexpr int kMaxSynchronousHighlightCatchUpLines = 256;

struct HighlightSegment {
    int start = 0;
    int length = 0;
    QColor color;
};

QFont editorFont()
{
    QFont font(QStringLiteral("Consolas"));
    font.setPixelSize(static_cast<int>(kFontPixelSize));
    font.setStyleHint(QFont::Monospace);
    font.setFixedPitch(true);
    return font;
}

int tabAdvance(int column)
{
    return kTabColumns - (column % kTabColumns);
}

int textLength(const QString& text)
{
    return static_cast<int>(text.size());
}

KSyntaxHighlighting::Repository& syntaxRepository()
{
    static auto* repository = new KSyntaxHighlighting::Repository;
    return *repository;
}

QString normalizedThemeName(const QString& themeId)
{
    if (themeId == QStringLiteral("github-dark")) {
        return QStringLiteral("GitHub Dark");
    }
    if (themeId == QStringLiteral("github-light")) {
        return QStringLiteral("GitHub Light");
    }
    if (themeId == QStringLiteral("darcula")) {
        return QStringLiteral("Dracula");
    }
    if (themeId == QStringLiteral("monokai")) {
        return QStringLiteral("Monokai");
    }
    return themeId;
}

KSyntaxHighlighting::Theme themeForId(const QString& themeId, bool darkFallback)
{
    auto& repository = syntaxRepository();
    auto theme = repository.theme(normalizedThemeName(themeId));
    if (theme.isValid()) {
        return theme;
    }

    return repository.defaultTheme(darkFallback
        ? KSyntaxHighlighting::Repository::DarkTheme
        : KSyntaxHighlighting::Repository::LightTheme);
}

KSyntaxHighlighting::Definition definitionForSyntax(const QString& syntax)
{
    auto& repository = syntaxRepository();
    const QString trimmed = syntax.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    if (trimmed.compare(QStringLiteral("JSON"), Qt::CaseInsensitive) == 0) {
        return repository.definitionForName(QStringLiteral("JSON"));
    }

    const QString lower = trimmed.toLower();
    if (lower.endsWith(QStringLiteral(".ets"))) {
        const auto definition = repository.definitionForName(QStringLiteral("TypeScript"));
        if (definition.isValid()) {
            return definition;
        }
    }

    const QFileInfo fileInfo(trimmed);
    auto definition = repository.definitionForFileName(fileInfo.fileName());
    if (definition.isValid()) {
        return definition;
    }

    definition = repository.definitionForName(trimmed);
    if (definition.isValid()) {
        return definition;
    }

    return {};
}

} // namespace

class CodeLineHighlighter : public KSyntaxHighlighting::AbstractHighlighter {
public:
    using LineProvider = std::function<QStringView(int)>;

    void setInputs(const QString& syntax, const QString& themeId, bool darkTheme)
    {
        setTheme(themeForId(themeId, darkTheme));
        setDefinition(definitionForSyntax(syntax));
        reset();
    }

    void setEnabled(bool enabled)
    {
        if (enabled_ == enabled) {
            return;
        }
        enabled_ = enabled;
        reset();
    }

    void reset()
    {
        cachedLineCount_ = 0;
        states_.clear();
        states_.append(KSyntaxHighlighting::State());
    }

    [[nodiscard]] QVector<HighlightSegment> segmentsForLine(int line, int lineCount, const LineProvider& lineProvider)
    {
        segments_.clear();
        if (!enabled_ || !definition().isValid() || !theme().isValid()) {
            return {};
        }

        if (states_.size() < lineCount + 1) {
            states_.resize(lineCount + 1);
        }

        if (cachedLineCount_ <= line) {
            if (line - cachedLineCount_ > kMaxSynchronousHighlightCatchUpLines) {
                return {};
            }
            for (int i = cachedLineCount_; i <= line; ++i) {
                segments_.clear();
                states_[i + 1] = highlightLine(lineProvider(i), states_.at(i));
            }
            cachedLineCount_ = line + 1;
            return segments_;
        }

        states_[line + 1] = highlightLine(lineProvider(line), states_.at(line));
        return segments_;
    }

protected:
    void applyFormat(int offset, int length, const KSyntaxHighlighting::Format& format) override
    {
        if (length <= 0 || !format.isValid() || !format.hasTextColor(theme())) {
            return;
        }

        segments_.append(HighlightSegment {
            offset,
            length,
            format.textColor(theme())
        });
    }

private:
    bool enabled_ = false;
    int cachedLineCount_ = 0;
    QVector<KSyntaxHighlighting::State> states_ { KSyntaxHighlighting::State() };
    QVector<HighlightSegment> segments_;
};

CodeEditorItem::CodeEditorItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(false);
    setOpaquePainting(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    setFlag(QQuickItem::ItemIsFocusScope, true);

    refreshPalette();
    refreshTextMetrics();
    syntaxHighlighter_ = std::make_unique<CodeLineHighlighter>();
    refreshSyntaxHighlighter();
    rebuildDocument();
}

CodeEditorItem::~CodeEditorItem() = default;

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
    refreshSyntaxHighlighter();
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
    refreshSyntaxHighlighter();
    update();
    emit highlightThemeChanged();
}

QString CodeEditorItem::syntax() const
{
    return syntax_;
}

void CodeEditorItem::setSyntax(const QString& syntax)
{
    if (syntax_ == syntax) {
        return;
    }

    syntax_ = syntax;
    refreshSyntaxHighlighter();
    update();
    emit syntaxChanged();
}

bool CodeEditorItem::fastScrolling() const
{
    return fastScrolling_;
}

void CodeEditorItem::setFastScrolling(bool fastScrolling)
{
    if (fastScrolling_ == fastScrolling) {
        return;
    }

    fastScrolling_ = fastScrolling;
    update();
    emit fastScrollingChanged();
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
    return selectionAnchor_ >= 0 && selectionPosition_ >= 0 && selectionAnchor_ != selectionPosition_;
}

void CodeEditorItem::copySelection() const
{
    if (!hasSelection()) {
        return;
    }

    const int start = std::min(selectionAnchor_, selectionPosition_);
    const int end = std::max(selectionAnchor_, selectionPosition_);
    QGuiApplication::clipboard()->setText(text_.mid(start, end - start));
}

void CodeEditorItem::selectAll()
{
    const bool previousHasSelection = hasSelection();
    selectionAnchor_ = 0;
    selectionPosition_ = textLength(text_);
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

    const int currentLine = selectionPosition_ >= 0 ? lineForPosition(selectionPosition_) : -1;
    const bool hasTextSelection = hasSelection();
    const int selectionStart = hasTextSelection ? std::min(selectionAnchor_, selectionPosition_) : -1;
    const int selectionEnd = hasTextSelection ? std::max(selectionAnchor_, selectionPosition_) : -1;
    const int firstVisibleLine = std::max(0, static_cast<int>(std::floor(scrollY_ / lineHeight_)) - 1);
    const int lastVisibleLine = std::min(lineCount() - 1,
        static_cast<int>(std::ceil((scrollY_ + height() - kTopPadding) / lineHeight_)) + 1);

    painter->setClipRect(boundingRect());
    for (int line = firstVisibleLine; line <= lastVisibleLine; ++line) {
        const qreal y = kTopPadding + static_cast<qreal>(line) * lineHeight_ - scrollY_;
        if (y > height()) {
            break;
        }
        if (y + lineHeight_ < 0.0) {
            continue;
        }

        if (line == currentLine && hasActiveFocus()) {
            painter->fillRect(QRectF(gutter, y, width() - gutter, lineHeight_), currentLineColor_);
        }

        const QRectF numberRect(0.0, y, gutter - kGutterPadding, lineHeight_);
        painter->setPen(gutterTextColor_);
        painter->drawText(numberRect, Qt::AlignRight | Qt::AlignVCenter, QString::number(line + 1));

        if (hasTextSelection) {
            const int blockStart = lineStartPosition(line);
            const int blockEnd = lineEndPosition(line);
            const int start = std::max(selectionStart, blockStart);
            const int end = std::min(selectionEnd, blockEnd);
            if (start < end) {
                const int startColumn = visualColumnForPosition(line, start);
                const int endColumn = visualColumnForPosition(line, end);
                const qreal selectionX = gutter + kLeftPadding - scrollX_
                    + static_cast<qreal>(startColumn) * characterWidth_;
                const qreal selectionWidth = static_cast<qreal>(endColumn - startColumn) * characterWidth_;
                painter->fillRect(QRectF(selectionX, y, selectionWidth, lineHeight_), selectionColor_);
            } else if (selectionStart <= blockEnd && selectionEnd > blockEnd && blockEnd == blockStart) {
                const qreal selectionX = gutter + kLeftPadding - scrollX_;
                painter->fillRect(QRectF(selectionX, y, characterWidth_, lineHeight_), selectionColor_);
            }
        }

        painter->save();
        painter->setClipRect(QRectF(gutter, 0.0, width() - gutter, height()));

        const QStringView text = lineView(line);
        const int lineStart = lineStartPosition(line);
        const int visibleStartColumn = std::max(0, static_cast<int>(std::floor(scrollX_ / characterWidth_)) - 2);
        const int visibleEndColumn = std::max(visibleStartColumn,
            static_cast<int>(std::ceil((scrollX_ + width() - gutter - kLeftPadding) / characterWidth_)) + 2);
        const int visibleStart = positionForVisualColumn(line, visibleStartColumn) - lineStart;
        const int visibleEnd = positionForVisualColumn(line, visibleEndColumn) - lineStart;
        int drawnUntil = visibleStart;
        const auto drawRange = [&](int start, int end, const QColor& color) {
            start = std::clamp(start, 0, static_cast<int>(text.size()));
            end = std::clamp(end, start, static_cast<int>(text.size()));
            start = std::max(start, visibleStart);
            end = std::min(end, visibleEnd);
            if (start >= end) {
                return;
            }

            const int column = visualColumnForPosition(line, lineStart + start);
            painter->setPen(color);
            painter->drawText(QPointF(
                                  gutter + kLeftPadding - scrollX_ + static_cast<qreal>(column) * characterWidth_,
                                  y + lineAscent_),
                expandedTabs(text.mid(start, end - start)));
        };

        const auto segments = !fastScrolling_ && syntaxHighlighter_ != nullptr
            ? syntaxHighlighter_->segmentsForLine(line, lineCount(), [this](int requestedLine) {
                  return lineView(requestedLine);
              })
            : QVector<HighlightSegment>();
        for (const HighlightSegment& segment : segments) {
            if (segment.start >= visibleEnd) {
                break;
            }
            if (segment.start + segment.length <= visibleStart) {
                continue;
            }
            if (segment.start > drawnUntil) {
                drawRange(drawnUntil, segment.start, textColor_);
            }
            drawRange(segment.start, segment.start + segment.length, segment.color);
            drawnUntil = std::max(drawnUntil, segment.start + segment.length);
        }
        drawRange(drawnUntil, visibleEnd, textColor_);
        painter->restore();
    }
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
    const int position = positionAt(event->position());
    int start = position;
    int end = position;
    while (start > 0 && isWordCharacter(text_.at(start - 1))) {
        --start;
    }
    while (end < textLength(text_) && isWordCharacter(text_.at(end))) {
        ++end;
    }
    if (start == end && position < textLength(text_)) {
        end = position + 1;
    }

    selectionAnchor_ = start;
    selectionPosition_ = end;
    selecting_ = false;
    lastDoubleClickPoint_ = event->position();
    lastDoubleClickBlock_ = lineForPosition(selectionPosition_);
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

    const bool keepAnchor = event->modifiers().testFlag(Qt::ShiftModifier);
    const bool control = event->modifiers().testFlag(Qt::ControlModifier);

    switch (event->key()) {
    case Qt::Key_Left:
        moveCursor(control ? CursorMove::PreviousWord : CursorMove::Left, keepAnchor);
        event->accept();
        return;
    case Qt::Key_Right:
        moveCursor(control ? CursorMove::NextWord : CursorMove::Right, keepAnchor);
        event->accept();
        return;
    case Qt::Key_Up:
        moveCursor(CursorMove::Up, keepAnchor);
        event->accept();
        return;
    case Qt::Key_Down:
        moveCursor(CursorMove::Down, keepAnchor);
        event->accept();
        return;
    case Qt::Key_Home:
        moveCursor(control ? CursorMove::DocumentStart : CursorMove::LineStart, keepAnchor);
        event->accept();
        return;
    case Qt::Key_End:
        moveCursor(control ? CursorMove::DocumentEnd : CursorMove::LineEnd, keepAnchor);
        event->accept();
        return;
    case Qt::Key_PageUp:
        moveCursorByPage(-1, keepAnchor);
        event->accept();
        return;
    case Qt::Key_PageDown:
        moveCursorByPage(1, keepAnchor);
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
    PerformanceTrace trace(QStringLiteral("CodeEditorItem::rebuildDocument"));

    refreshTextMetrics();
    rebuildLineIndex();
    refreshSyntaxHighlighter();
    selectionAnchor_ = -1;
    selectionPosition_ = -1;
    selecting_ = false;
    refreshMetrics();
    update();
}

void CodeEditorItem::rebuildLineIndex()
{
    lineStarts_.clear();
    lineStarts_.reserve(std::max(1, static_cast<int>(text_.count(QLatin1Char('\n'))) + 1));
    lineStarts_.append(0);

    int currentColumns = 0;
    longestLineColumns_ = 0;
    for (int i = 0; i < textLength(text_); ++i) {
        const QChar ch = text_.at(i);
        if (ch == QLatin1Char('\n')) {
            longestLineColumns_ = std::max(longestLineColumns_, currentColumns);
            currentColumns = 0;
            if (i + 1 < textLength(text_)) {
                lineStarts_.append(i + 1);
            }
        } else if (ch == QLatin1Char('\t')) {
            currentColumns += tabAdvance(currentColumns);
        } else {
            ++currentColumns;
        }
    }
    longestLineColumns_ = std::max(longestLineColumns_, currentColumns);

    if (lineStarts_.isEmpty()) {
        lineStarts_.append(0);
    }
}

void CodeEditorItem::refreshSyntaxHighlighter()
{
    if (syntaxHighlighter_ == nullptr) {
        return;
    }

    syntaxHighlighter_->setInputs(syntax_, highlightTheme_, darkTheme_);
    syntaxHighlighter_->setEnabled(text_.size() <= kHighlightCharacterLimit && lineCount() <= kHighlightLineLimit);
}

void CodeEditorItem::refreshMetrics()
{
    const qreal oldWidth = documentWidth_;
    const qreal oldHeight = documentHeight_;
    documentWidth_ = gutterWidth() + kLeftPadding + characterWidth_ * longestLineColumns_ + kRightPadding;
    documentHeight_ = kTopPadding + static_cast<qreal>(lineCount()) * lineHeight_ + kBottomPadding;

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

void CodeEditorItem::refreshTextMetrics()
{
    const QFontMetricsF metrics(editorFont());
    lineHeight_ = std::max<qreal>(1.0, metrics.lineSpacing());
    lineAscent_ = (lineHeight_ - metrics.height()) / 2.0 + metrics.ascent();
    characterWidth_ = std::max<qreal>(1.0, metrics.horizontalAdvance(QLatin1Char('M')));
}

void CodeEditorItem::setCursorPosition(int position, bool keepAnchor)
{
    position = std::clamp(position, 0, textLength(text_));
    if (!keepAnchor || selectionAnchor_ < 0) {
        selectionAnchor_ = position;
    }
    selectionPosition_ = position;
}

void CodeEditorItem::moveCursor(CursorMove operation, bool keepAnchor)
{
    const bool previousHasSelection = hasSelection();
    const int position = selectionPosition_ >= 0 ? selectionPosition_ : 0;
    setCursorPosition(movedPosition(position, operation), keepAnchor);
    update();
    notifySelectionChanged(previousHasSelection);
}

void CodeEditorItem::moveCursorByPage(int direction, bool keepAnchor)
{
    const bool previousHasSelection = hasSelection();
    const int position = selectionPosition_ >= 0 ? selectionPosition_ : 0;
    const int line = lineForPosition(position);
    const int column = visualColumnForPosition(line, position);
    const int targetLine = std::clamp(line + direction * visibleLineCount(), 0, lineCount() - 1);
    setCursorPosition(positionForVisualColumn(targetLine, column), keepAnchor);
    update();
    notifySelectionChanged(previousHasSelection);
}

void CodeEditorItem::selectLineAt(int position)
{
    const int line = lineForPosition(position);
    selectionAnchor_ = lineStartPosition(line);
    selectionPosition_ = lineEndPosition(line);
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
    const int block = lineForPosition(positionAt(point));
    return block >= 0 && block == lastDoubleClickBlock_;
}

int CodeEditorItem::visibleLineCount() const
{
    return std::max(1, static_cast<int>((height() - kTopPadding - kBottomPadding) / lineHeight_));
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
    const int line = std::clamp(static_cast<int>(std::floor((point.y() - kTopPadding + scrollY_) / lineHeight_)),
        0,
        lineCount() - 1);
    const qreal x = point.x() - gutter - kLeftPadding + scrollX_;
    const int visualColumn = std::max(0, static_cast<int>(std::floor((x + characterWidth_ * 0.5) / characterWidth_)));
    return positionForVisualColumn(line, visualColumn);
}

int CodeEditorItem::lineCount() const
{
    return std::max(1, static_cast<int>(lineStarts_.size()));
}

qreal CodeEditorItem::gutterWidth() const
{
    const int digits = QString::number(lineCount()).length();
    const QFontMetricsF metrics(editorFont());
    return std::max(kMinimumGutterWidth, kGutterPadding * 2.0 + metrics.horizontalAdvance(QString(digits, QLatin1Char('9'))));
}

int CodeEditorItem::lineForPosition(int position) const
{
    if (lineStarts_.isEmpty()) {
        return 0;
    }

    position = std::clamp(position, 0, textLength(text_));
    const auto it = std::upper_bound(lineStarts_.cbegin(), lineStarts_.cend(), position);
    if (it == lineStarts_.cbegin()) {
        return 0;
    }
    return static_cast<int>(std::distance(lineStarts_.cbegin(), it)) - 1;
}

int CodeEditorItem::lineStartPosition(int line) const
{
    if (lineStarts_.isEmpty()) {
        return 0;
    }
    line = std::clamp(line, 0, lineCount() - 1);
    return lineStarts_.at(line);
}

int CodeEditorItem::lineEndPosition(int line) const
{
    line = std::clamp(line, 0, lineCount() - 1);
    if (line + 1 < static_cast<int>(lineStarts_.size())) {
        return std::max(lineStarts_.at(line), lineStarts_.at(line + 1) - 1);
    }
    return textLength(text_);
}

QStringView CodeEditorItem::lineView(int line) const
{
    const int start = lineStartPosition(line);
    const int end = lineEndPosition(line);
    return QStringView(text_).mid(start, end - start);
}

int CodeEditorItem::visualColumnForPosition(int line, int position) const
{
    const int start = lineStartPosition(line);
    const int end = lineEndPosition(line);
    position = std::clamp(position, start, end);

    int column = 0;
    for (int i = start; i < position; ++i) {
        const QChar ch = text_.at(i);
        column += ch == QLatin1Char('\t') ? tabAdvance(column) : 1;
    }
    return column;
}

int CodeEditorItem::positionForVisualColumn(int line, int visualColumn) const
{
    const int start = lineStartPosition(line);
    const int end = lineEndPosition(line);
    visualColumn = std::max(0, visualColumn);

    int column = 0;
    for (int i = start; i < end; ++i) {
        const QChar ch = text_.at(i);
        const int advance = ch == QLatin1Char('\t') ? tabAdvance(column) : 1;
        if (visualColumn < column + advance) {
            return i;
        }
        column += advance;
    }
    return end;
}

int CodeEditorItem::movedPosition(int position, CursorMove operation) const
{
    position = std::clamp(position, 0, textLength(text_));
    const int line = lineForPosition(position);

    switch (operation) {
    case CursorMove::Left:
        return std::max(0, position - 1);
    case CursorMove::Right:
        return std::min(textLength(text_), position + 1);
    case CursorMove::Up: {
        const int column = visualColumnForPosition(line, position);
        return positionForVisualColumn(std::max(0, line - 1), column);
    }
    case CursorMove::Down: {
        const int column = visualColumnForPosition(line, position);
        return positionForVisualColumn(std::min(lineCount() - 1, line + 1), column);
    }
    case CursorMove::LineStart:
        return lineStartPosition(line);
    case CursorMove::LineEnd:
        return lineEndPosition(line);
    case CursorMove::DocumentStart:
        return 0;
    case CursorMove::DocumentEnd:
        return textLength(text_);
    case CursorMove::PreviousWord:
        return previousWordPosition(position);
    case CursorMove::NextWord:
        return nextWordPosition(position);
    }

    return position;
}

int CodeEditorItem::previousWordPosition(int position) const
{
    position = std::clamp(position, 0, textLength(text_));
    while (position > 0 && text_.at(position - 1).isSpace()) {
        --position;
    }
    if (position > 0 && isWordCharacter(text_.at(position - 1))) {
        while (position > 0 && isWordCharacter(text_.at(position - 1))) {
            --position;
        }
        return position;
    }
    while (position > 0 && !text_.at(position - 1).isSpace() && !isWordCharacter(text_.at(position - 1))) {
        --position;
    }
    return position;
}

int CodeEditorItem::nextWordPosition(int position) const
{
    position = std::clamp(position, 0, textLength(text_));
    if (position < textLength(text_) && isWordCharacter(text_.at(position))) {
        while (position < textLength(text_) && isWordCharacter(text_.at(position))) {
            ++position;
        }
    } else {
        while (position < textLength(text_) && !text_.at(position).isSpace() && !isWordCharacter(text_.at(position))) {
            ++position;
        }
    }
    while (position < textLength(text_) && text_.at(position).isSpace()) {
        ++position;
    }
    return position;
}

bool CodeEditorItem::isWordCharacter(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

QString CodeEditorItem::expandedTabs(QStringView text)
{
    QString expanded;
    expanded.reserve(text.size());

    int column = 0;
    for (const QChar ch : text) {
        if (ch == QLatin1Char('\t')) {
            const int spaces = tabAdvance(column);
            expanded.append(QString(spaces, QLatin1Char(' ')));
            column += spaces;
        } else {
            expanded.append(ch);
            ++column;
        }
    }
    return expanded;
}
