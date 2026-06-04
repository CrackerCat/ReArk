#ifndef REARK_APP_INITIALIZER_H
#define REARK_APP_INITIALIZER_H

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QString>

class DecompilerController;

class AppInitializer {
public:
    AppInitializer(QGuiApplication& app, QQmlApplicationEngine& engine, QString initialFileUrl);

    void initializeAll();
    static void autoDisableDebugOutput();

private:
    void initializeApplication();
    void initializeContext();
    void initializeQmlModules();

    QGuiApplication& app_;
    QQmlApplicationEngine& engine_;
    QString initialFileUrl_;
    DecompilerController* decompilerController_ = nullptr;
};

#endif // REARK_APP_INITIALIZER_H
