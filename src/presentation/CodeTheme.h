#ifndef REARK_CODE_THEME_H
#define REARK_CODE_THEME_H

#include <QColor>
#include <QString>

struct CodeTheme {
    QString id;
    QColor editor;
    QColor text;
    QColor gutter;
    QColor gutterText;
    QColor divider;
    QColor currentLine;
    QColor selection;
    QColor selectedText;
    QColor keyword;
    QColor type;
    QColor string;
    QColor comment;
    QColor number;
};

CodeTheme codeThemeForId(const QString& id, bool darkFallback);

#endif // REARK_CODE_THEME_H
