#include "core/AppInitializer.h"

#include "controller/DecompilerController.h"

#include <QIcon>
#include <QQmlContext>
#include <QQuickStyle>

#include <utility>

AppInitializer::AppInitializer(QGuiApplication& app, QQmlApplicationEngine& engine, QString initialFileUrl)
    : app_(app)
    , engine_(engine)
    , initialFileUrl_(std::move(initialFileUrl))
{
}

void AppInitializer::initializeAll()
{
    initializeApplication();
    initializeContext();
    initializeQmlModules();
}

void AppInitializer::autoDisableDebugOutput()
{
#ifdef QT_NO_DEBUG_OUTPUT
    qputenv("QT_LOGGING_RULES", QByteArray("qml=false"));
#endif
}

void AppInitializer::initializeApplication()
{
    QGuiApplication::setApplicationName(QStringLiteral("ReArk"));
    QGuiApplication::setOrganizationName(QStringLiteral("ReArk"));
    QGuiApplication::setApplicationVersion(QStringLiteral(REARK_VERSION));
    QGuiApplication::setWindowIcon(QIcon(QStringLiteral(":/images/app_icon.ico")));
    QQuickStyle::setStyle(QStringLiteral("Material"));
}

void AppInitializer::initializeContext()
{
    decompilerController_ = new DecompilerController(&engine_);

    auto* context = engine_.rootContext();
    context->setContextProperty(QStringLiteral("appVersion"), QStringLiteral(REARK_VERSION));
    context->setContextProperty(QStringLiteral("initialFileUrl"), initialFileUrl_);
    context->setContextProperty(QStringLiteral("decompilerController"), decompilerController_);
}

void AppInitializer::initializeQmlModules()
{
    engine_.addImportPath(QStringLiteral("qrc:/"));
}
