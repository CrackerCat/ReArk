#include "presentation/CodeTheme.h"

CodeTheme codeThemeForId(const QString& id, bool darkFallback)
{
    if (id == QStringLiteral("github-dark")) {
        return {
            id,
            QColor(QStringLiteral("#0d1117")),
            QColor(QStringLiteral("#c9d1d9")),
            QColor(QStringLiteral("#161b22")),
            QColor(QStringLiteral("#6e7681")),
            QColor(QStringLiteral("#30363d")),
            QColor(QStringLiteral("#161b22")),
            QColor(QStringLiteral("#264f78")),
            QColor(QStringLiteral("#ffffff")),
            QColor(QStringLiteral("#ff7b72")),
            QColor(QStringLiteral("#ffa657")),
            QColor(QStringLiteral("#a5d6ff")),
            QColor(QStringLiteral("#8b949e")),
            QColor(QStringLiteral("#79c0ff"))
        };
    }

    if (id == QStringLiteral("github-light")) {
        return {
            id,
            QColor(QStringLiteral("#ffffff")),
            QColor(QStringLiteral("#24292f")),
            QColor(QStringLiteral("#f6f8fa")),
            QColor(QStringLiteral("#6e7781")),
            QColor(QStringLiteral("#d0d7de")),
            QColor(QStringLiteral("#f6f8fa")),
            QColor(QStringLiteral("#b6e3ff")),
            QColor(QStringLiteral("#24292f")),
            QColor(QStringLiteral("#cf222e")),
            QColor(QStringLiteral("#953800")),
            QColor(QStringLiteral("#0a3069")),
            QColor(QStringLiteral("#6e7781")),
            QColor(QStringLiteral("#0550ae"))
        };
    }

    if (id == QStringLiteral("monokai")) {
        return {
            id,
            QColor(QStringLiteral("#272822")),
            QColor(QStringLiteral("#f8f8f2")),
            QColor(QStringLiteral("#1f201b")),
            QColor(QStringLiteral("#75715e")),
            QColor(QStringLiteral("#3e3d32")),
            QColor(QStringLiteral("#32332a")),
            QColor(QStringLiteral("#49483e")),
            QColor(QStringLiteral("#f8f8f2")),
            QColor(QStringLiteral("#f92672")),
            QColor(QStringLiteral("#66d9ef")),
            QColor(QStringLiteral("#e6db74")),
            QColor(QStringLiteral("#75715e")),
            QColor(QStringLiteral("#ae81ff"))
        };
    }

    if (id == QStringLiteral("darcula")) {
        return {
            id,
            QColor(QStringLiteral("#2b2b2b")),
            QColor(QStringLiteral("#a9b7c6")),
            QColor(QStringLiteral("#313335")),
            QColor(QStringLiteral("#606366")),
            QColor(QStringLiteral("#3c3f41")),
            QColor(QStringLiteral("#323232")),
            QColor(QStringLiteral("#214283")),
            QColor(QStringLiteral("#ffffff")),
            QColor(QStringLiteral("#cc7832")),
            QColor(QStringLiteral("#ffc66d")),
            QColor(QStringLiteral("#6a8759")),
            QColor(QStringLiteral("#808080")),
            QColor(QStringLiteral("#6897bb"))
        };
    }

    if (!darkFallback) {
        return codeThemeForId(QStringLiteral("github-light"), true);
    }

    return codeThemeForId(QStringLiteral("github-dark"), true);
}
