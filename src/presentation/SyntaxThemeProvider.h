#ifndef REARK_SYNTAX_THEME_PROVIDER_H
#define REARK_SYNTAX_THEME_PROVIDER_H

#include <QObject>
#include <QQmlEngine>
#include <QStringList>

class SyntaxThemeProvider : public QObject {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QStringList themes READ themes CONSTANT)

public:
    explicit SyntaxThemeProvider(QObject* parent = nullptr);

    [[nodiscard]] QStringList themes() const;

private:
    QStringList themes_;
};

#endif // REARK_SYNTAX_THEME_PROVIDER_H
