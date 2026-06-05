#include "presentation/SyntaxThemeProvider.h"

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

#include <algorithm>

SyntaxThemeProvider::SyntaxThemeProvider(QObject* parent)
    : QObject(parent)
{
    KSyntaxHighlighting::Repository repository;
    const auto themes = repository.themes();
    themes_.reserve(themes.size());
    for (const auto& theme : themes) {
        if (theme.isValid()) {
            themes_.append(theme.name());
        }
    }

    themes_.removeDuplicates();
    std::sort(themes_.begin(), themes_.end(), [](const QString& lhs, const QString& rhs) {
        return QString::localeAwareCompare(lhs, rhs) < 0;
    });
}

QStringList SyntaxThemeProvider::themes() const
{
    return themes_;
}
