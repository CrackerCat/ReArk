#include "presentation/SyntaxHighlighter.h"

namespace {

QTextCharFormat makeFormat(const QColor& color, QFont::Weight weight = QFont::Normal)
{
    QTextCharFormat result;
    result.setForeground(color);
    result.setFontWeight(weight);
    return result;
}

} // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : QSyntaxHighlighter(parent)
{
    rebuildRules();
}

void SyntaxHighlighter::setTheme(const QString& themeId, bool darkFallback)
{
    if (themeId_ == themeId && darkTheme_ == darkFallback) {
        return;
    }

    themeId_ = themeId;
    darkTheme_ = darkFallback;
    rebuildRules();
    rehighlight();
}

void SyntaxHighlighter::rebuildRules()
{
    rules_.clear();
    const CodeTheme theme = codeThemeForId(themeId_, darkTheme_);

    keywordFormat_ = makeFormat(theme.keyword, QFont::DemiBold);
    typeFormat_ = makeFormat(theme.type);
    stringFormat_ = makeFormat(theme.string);
    commentFormat_ = makeFormat(theme.comment);
    numberFormat_ = makeFormat(theme.number);

    const QStringList keywords = {
        QStringLiteral("abstract"), QStringLiteral("as"), QStringLiteral("async"),
        QStringLiteral("await"), QStringLiteral("break"), QStringLiteral("case"),
        QStringLiteral("catch"), QStringLiteral("class"), QStringLiteral("const"),
        QStringLiteral("continue"), QStringLiteral("default"), QStringLiteral("do"),
        QStringLiteral("else"), QStringLiteral("enum"), QStringLiteral("export"),
        QStringLiteral("extends"), QStringLiteral("false"), QStringLiteral("finally"),
        QStringLiteral("for"), QStringLiteral("from"), QStringLiteral("function"),
        QStringLiteral("if"), QStringLiteral("import"), QStringLiteral("in"),
        QStringLiteral("let"), QStringLiteral("new"), QStringLiteral("null"),
        QStringLiteral("private"), QStringLiteral("protected"), QStringLiteral("public"),
        QStringLiteral("return"), QStringLiteral("static"), QStringLiteral("struct"),
        QStringLiteral("super"), QStringLiteral("switch"), QStringLiteral("this"),
        QStringLiteral("throw"), QStringLiteral("true"), QStringLiteral("try"),
        QStringLiteral("type"), QStringLiteral("undefined"), QStringLiteral("var"),
        QStringLiteral("void"), QStringLiteral("while")
    };

    for (const auto& keyword : keywords) {
        rules_.push_back({
            QRegularExpression(QStringLiteral("\\b%1\\b").arg(keyword)),
            keywordFormat_
        });
    }

    const QStringList types = {
        QStringLiteral("Array"), QStringLiteral("Date"), QStringLiteral("Map"),
        QStringLiteral("Object"), QStringLiteral("Promise"), QStringLiteral("Record"),
        QStringLiteral("Set"), QStringLiteral("boolean"), QStringLiteral("number"),
        QStringLiteral("string")
    };
    for (const auto& type : types) {
        rules_.push_back({
            QRegularExpression(QStringLiteral("\\b%1\\b").arg(type)),
            typeFormat_
        });
    }

    rules_.push_back({ QRegularExpression(QStringLiteral("\\b[0-9][A-Za-z0-9_.]*\\b")), numberFormat_ });
    rules_.push_back({ QRegularExpression(QStringLiteral("'([^'\\\\]|\\\\.)*'")), stringFormat_ });
    rules_.push_back({ QRegularExpression(QStringLiteral("\"([^\"\\\\]|\\\\.)*\"")), stringFormat_ });
    rules_.push_back({ QRegularExpression(QStringLiteral("`([^`\\\\]|\\\\.)*`")), stringFormat_ });
    rules_.push_back({ QRegularExpression(QStringLiteral("//[^\\n]*")), commentFormat_ });
}

void SyntaxHighlighter::highlightBlock(const QString& text)
{
    for (const auto& rule : rules_) {
        auto it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            const auto match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);

    int start = 0;
    if (previousBlockState() != 1) {
        start = text.indexOf(QStringLiteral("/*"));
    }

    while (start >= 0) {
        const int end = text.indexOf(QStringLiteral("*/"), start + 2);
        int length = 0;
        if (end == -1) {
            setCurrentBlockState(1);
            length = text.length() - start;
        } else {
            length = end - start + 2;
        }
        setFormat(start, length, commentFormat_);
        start = text.indexOf(QStringLiteral("/*"), start + length);
    }
}
