#ifndef REARK_SYNTAX_HIGHLIGHTER_H
#define REARK_SYNTAX_HIGHLIGHTER_H

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>

class SyntaxHighlighter : public KSyntaxHighlighting::SyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument* parent = nullptr);
    void setTheme(const QString& themeId, bool darkFallback);
    void setSyntax(const QString& syntax);
    void setHighlightingEnabled(bool enabled);

private:
    void refreshDefinition();
    void refreshTheme();
    [[nodiscard]] KSyntaxHighlighting::Definition definitionForSyntax(const QString& syntax) const;
    [[nodiscard]] KSyntaxHighlighting::Theme themeForId(const QString& themeId, bool darkFallback) const;
    [[nodiscard]] QString normalizedThemeName(const QString& themeId) const;

    bool highlightingEnabled_ = true;
    bool darkTheme_ = true;
    QString themeId_ = QStringLiteral("GitHub Dark");
    QString syntax_;
};

#endif // REARK_SYNTAX_HIGHLIGHTER_H
