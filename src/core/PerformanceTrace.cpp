#include "core/PerformanceTrace.h"

#include <QDebug>
#include <QLoggingCategory>
#include <QtGlobal>

#include <utility>

Q_LOGGING_CATEGORY(rearkPerfLog, "reark.perf")

PerformanceTrace::PerformanceTrace(QString name)
    : name_(std::move(name))
    , enabled_(qEnvironmentVariableIsSet("REARK_PERF"))
{
    if (enabled_) {
        timer_.start();
    }
}

PerformanceTrace::~PerformanceTrace()
{
    if (enabled_) {
        qCInfo(rearkPerfLog).noquote() << name_ << "took" << timer_.elapsed() << "ms";
    }
}
