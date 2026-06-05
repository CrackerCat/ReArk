#include "presentation/SyntaxHighlighter.h"

#include <KSyntaxHighlighting/Repository>

#include <QFileInfo>

namespace {

KSyntaxHighlighting::Repository& syntaxRepository()
{
    static auto* repository = new KSyntaxHighlighting::Repository;
    return *repository;
}

} // namespace

SyntaxHighlighter::SyntaxHighlighter(QTextDocument* parent)
    : KSyntaxHighlighting::SyntaxHighlighter(parent)
{
    refreshTheme();
    refreshDefinition();
}

void SyntaxHighlighter::setTheme(const QString& themeId, bool darkFallback)
{
    if (themeId_ == themeId && darkTheme_ == darkFallback) {
        return;
    }

    themeId_ = themeId;
    darkTheme_ = darkFallback;
    refreshTheme();
}

void SyntaxHighlighter::setSyntax(const QString& syntax)
{
    if (syntax_ == syntax) {
        return;
    }

    syntax_ = syntax;
    refreshDefinition();
}

void SyntaxHighlighter::setHighlightingEnabled(bool enabled)
{
    if (highlightingEnabled_ == enabled) {
        return;
    }
    highlightingEnabled_ = enabled;
    refreshDefinition();
}

void SyntaxHighlighter::refreshDefinition()
{
    if (!highlightingEnabled_) {
        setDefinition(KSyntaxHighlighting::Definition());
        rehighlight();
        return;
    }

    setDefinition(definitionForSyntax(syntax_));
    rehighlight();
}

void SyntaxHighlighter::refreshTheme()
{
    KSyntaxHighlighting::SyntaxHighlighter::setTheme(themeForId(themeId_, darkTheme_));
}

KSyntaxHighlighting::Definition SyntaxHighlighter::definitionForSyntax(const QString& syntax) const
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

KSyntaxHighlighting::Theme SyntaxHighlighter::themeForId(const QString& themeId, bool darkFallback) const
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

QString SyntaxHighlighter::normalizedThemeName(const QString& themeId) const
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
