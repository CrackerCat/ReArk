#ifndef REARK_PERFORMANCE_TRACE_H
#define REARK_PERFORMANCE_TRACE_H

#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QString>

Q_DECLARE_LOGGING_CATEGORY(rearkPerfLog)

class PerformanceTrace {
public:
    explicit PerformanceTrace(QString name);
    ~PerformanceTrace();

private:
    QString name_;
    QElapsedTimer timer_;
    bool enabled_ = false;
};

#endif // REARK_PERFORMANCE_TRACE_H
