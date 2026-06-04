#ifndef REARK_HYLE_DECOMPILER_H
#define REARK_HYLE_DECOMPILER_H

#include "model/SourceTreeModel.h"

#include <hyle/hap/decompile/abc_decompiler.h>

#include <QString>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace HyleDecompiler {

using Session = hyle::hap::decompiled_package_session;

struct OpenResult {
    QString error;
    QString status;
    std::vector<DecompiledSourceFile> files;
    std::shared_ptr<Session> session;
    std::shared_ptr<std::mutex> sessionMutex;
};

struct SourceResult {
    int nodeIndex = -1;
    QString name;
    QString content;
    QString diagnostics;
    QString kind;
    QString contentMode = QStringLiteral("text");
    QString error;
};

[[nodiscard]] OpenResult openFile(const QString& filePath);
[[nodiscard]] SourceResult decompileSourceFile(
    const std::shared_ptr<Session>& session,
    const std::shared_ptr<std::mutex>& sessionMutex,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name);
[[nodiscard]] SourceResult readResourceContent(
    const std::shared_ptr<Session>& session,
    const std::shared_ptr<std::mutex>& sessionMutex,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name);

} // namespace HyleDecompiler

#endif // REARK_HYLE_DECOMPILER_H
