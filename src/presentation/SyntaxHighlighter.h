#ifndef REARK_SYNTAX_HIGHLIGHTER_H
#define REARK_SYNTAX_HIGHLIGHTER_H

#include "presentation/CodeTheme.h"

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class SyntaxHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument* parent = nullptr);
    void setTheme(const QString& themeId, bool darkFallback);

protected:
    void highlightBlock(const QString& text) override;

private:
    struct Rule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };

    void rebuildRules();

    QVector<Rule> rules_;
    bool darkTheme_ = true;
    QString themeId_ = QStringLiteral("github-dark");
    QTextCharFormat keywordFormat_;
    QTextCharFormat typeFormat_;
    QTextCharFormat stringFormat_;
    QTextCharFormat commentFormat_;
    QTextCharFormat numberFormat_;
};

#endif // REARK_SYNTAX_HIGHLIGHTER_H
