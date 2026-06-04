#include "core/HyleDecompiler.h"

#include <hyle.h>

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeDatabase>
#include <QMimeType>
#include <QStringList>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <system_error>

namespace {

QString fromUtf8(const std::string& value)
{
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::string toLower(std::string value)
{
    std::ranges::transform(value, value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

QString errorMessage(const char* context, const std::error_code& error)
{
    return QStringLiteral("%1: %2").arg(QString::fromLatin1(context), fromUtf8(error.message()));
}

std::string toUtf8Path(const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    return std::string(bytes.constData(), static_cast<std::size_t>(bytes.size()));
}

bool isAbcFile(const QString& filePath)
{
    return QFileInfo(filePath).suffix().compare(QStringLiteral("abc"), Qt::CaseInsensitive) == 0;
}

QString normalizeSourceContent(QString content);

QString languageKind(const std::string& language)
{
    if (language.empty()) {
        return QStringLiteral("SRC");
    }
    return QString::fromStdString(toLower(language)).toUpper();
}

QString resourceKindName(hyle::hap::hap_resource_kind kind)
{
    using hyle::hap::hap_resource_kind;
    switch (kind) {
    case hap_resource_kind::directory:
        return QStringLiteral("DIR");
    case hap_resource_kind::abc:
        return QStringLiteral("ABC");
    case hap_resource_kind::json:
        return QStringLiteral("JSON");
    case hap_resource_kind::image:
        return QStringLiteral("IMAGE");
    case hap_resource_kind::media:
        return QStringLiteral("MEDIA");
    case hap_resource_kind::native_library:
        return QStringLiteral("LIB");
    case hap_resource_kind::signature:
        return QStringLiteral("SIGNATURE");
    case hap_resource_kind::resource_index:
        return QStringLiteral("RESOURCE_INDEX");
    case hap_resource_kind::text:
        return QStringLiteral("TXT");
    case hap_resource_kind::binary:
        return QStringLiteral("BIN");
    case hap_resource_kind::unknown:
        return QStringLiteral("UNKNOWN");
    }
    return QStringLiteral("UNKNOWN");
}

bool isTextResource(hyle::hap::hap_resource_kind kind)
{
    using hyle::hap::hap_resource_kind;
    return kind == hap_resource_kind::json
        || kind == hap_resource_kind::resource_index
        || kind == hap_resource_kind::signature
        || kind == hap_resource_kind::text;
}

bool isImageResource(hyle::hap::hap_resource_kind kind)
{
    return kind == hyle::hap::hap_resource_kind::image;
}

bool isImageMime(const QMimeType& mime)
{
    return mime.isValid() && mime.name().startsWith(QStringLiteral("image/"));
}

bool isImageResource(hyle::hap::hap_resource_kind kind, const std::string& path)
{
    if (isImageResource(kind)) {
        return true;
    }

    QMimeDatabase mimeDatabase;
    return isImageMime(mimeDatabase.mimeTypeForFile(fromUtf8(path), QMimeDatabase::MatchExtension));
}

QString bytesToUtf8Text(const std::vector<std::byte>& bytes)
{
    if (bytes.empty()) {
        return {};
    }
    return QString::fromUtf8(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<qsizetype>(bytes.size()));
}

QByteArray bytesToByteArray(const std::vector<std::byte>& bytes)
{
    if (bytes.empty()) {
        return {};
    }
    return QByteArray(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<qsizetype>(bytes.size()));
}

QMimeType detectResourceMimeType(const hyle::hap::hap_resource_content& content)
{
    QMimeDatabase mimeDatabase;
    const auto bytes = bytesToByteArray(content.bytes);
    auto mime = mimeDatabase.mimeTypeForFileNameAndData(fromUtf8(content.path), bytes);
    if (!mime.isValid() || mime.name() == QStringLiteral("application/octet-stream")) {
        mime = mimeDatabase.mimeTypeForData(bytes);
    }
    return mime;
}

bool isImageResource(const hyle::hap::hap_resource_content& content)
{
    return isImageResource(content.kind, content.path) || isImageMime(detectResourceMimeType(content));
}

QString imageMimeType(const hyle::hap::hap_resource_content& content)
{
    const auto mime = detectResourceMimeType(content);
    return isImageMime(mime) ? mime.name() : QStringLiteral("image/png");
}

QString formatImageResource(const hyle::hap::hap_resource_content& content)
{
    const auto bytes = bytesToByteArray(content.bytes);
    return QStringLiteral("data:%1;base64,%2")
        .arg(imageMimeType(content), QString::fromLatin1(bytes.toBase64()));
}

QJsonArray buildHexRows(const std::vector<std::byte>& bytes)
{
    QJsonArray rows;
    for (std::size_t offset = 0; offset < bytes.size(); offset += 16U) {
        QJsonArray values;
        QString ascii;
        const auto lineSize = std::min<std::size_t>(16U, bytes.size() - offset);
        for (std::size_t i = 0; i < lineSize; ++i) {
            const auto value = std::to_integer<unsigned int>(bytes.at(offset + i));
            values.append(QStringLiteral("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper());
            ascii += value >= 32U && value <= 126U
                ? QLatin1Char(static_cast<char>(value))
                : QLatin1Char('.');
        }
        QJsonObject row;
        row.insert(QStringLiteral("address"),
            QStringLiteral("%1").arg(static_cast<qulonglong>(offset), 8, 16, QLatin1Char('0')).toUpper());
        row.insert(QStringLiteral("bytes"), values);
        row.insert(QStringLiteral("ascii"), ascii);
        rows.append(row);
    }
    return rows;
}

QString formatHexResource(const hyle::hap::hap_resource_content& content)
{
    QJsonObject root;
    root.insert(QStringLiteral("path"), fromUtf8(content.path));
    root.insert(QStringLiteral("kind"), resourceKindName(content.kind));
    root.insert(QStringLiteral("size"), QString::number(static_cast<qulonglong>(content.bytes.size())));
    root.insert(QStringLiteral("rows"), buildHexRows(content.bytes));
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QString boolText(bool value)
{
    return value ? QStringLiteral("true") : QStringLiteral("false");
}

QString signatureStatusText(hyle::hap::hap_signature_status status)
{
    using hyle::hap::hap_signature_status;
    switch (status) {
    case hap_signature_status::not_found:
        return QStringLiteral("not found");
    case hap_signature_status::profile_found:
        return QStringLiteral("profile found");
    case hap_signature_status::certificate_found:
        return QStringLiteral("certificate found");
    case hap_signature_status::certificate_parse_failed:
        return QStringLiteral("certificate parse failed");
    case hap_signature_status::malformed_profile:
        return QStringLiteral("malformed profile");
    }
    return QStringLiteral("unknown");
}

void appendField(QStringList& lines, const QString& name, const QString& value)
{
    if (!value.isEmpty()) {
        lines.append(QStringLiteral("%1: %2").arg(name, value));
    }
}

void appendField(QStringList& lines, const QString& name, bool value)
{
    lines.append(QStringLiteral("%1: %2").arg(name, boolText(value)));
}

void appendField(QStringList& lines, const QString& name, std::size_t value)
{
    if (value > 0) {
        lines.append(QStringLiteral("%1: %2").arg(name).arg(static_cast<qulonglong>(value)));
    }
}

QString formatCertificate(const hyle::hap::hap_certificate_info& certificate, int index)
{
    QStringList lines;
    lines.append(QStringLiteral("Certificate %1").arg(index + 1));
    lines.append(QStringLiteral("----------------"));
    appendField(lines, QStringLiteral("Status"), fromUtf8(certificate.status));
    appendField(lines, QStringLiteral("Status message"), fromUtf8(certificate.status_message));
    appendField(lines, QStringLiteral("Valid"), certificate.is_valid);
    appendField(lines, QStringLiteral("Signer"), fromUtf8(certificate.signer));
    appendField(lines, QStringLiteral("Issuer"), fromUtf8(certificate.issuer));
    appendField(lines, QStringLiteral("Valid from"), fromUtf8(certificate.valid_from));
    appendField(lines, QStringLiteral("Valid to"), fromUtf8(certificate.valid_to));
    appendField(lines, QStringLiteral("Algorithm"), fromUtf8(certificate.algorithm));
    appendField(lines, QStringLiteral("Key size"), certificate.key_size);
    appendField(lines, QStringLiteral("Thumbprint"), fromUtf8(certificate.thumbprint));

    if (!certificate.fingerprints.empty()) {
        lines.append(QStringLiteral("Fingerprints:"));
        for (const auto& fingerprint : certificate.fingerprints) {
            lines.append(QStringLiteral("  - %1").arg(fromUtf8(fingerprint)));
        }
    }

    return lines.join(QLatin1Char('\n'));
}

QString formatSignatureInfo(const hyle::hap::hap_signature_info& signature)
{
    QStringList lines;
    lines.append(QStringLiteral("HAP Signature"));
    lines.append(QStringLiteral("============="));
    lines.append(QString());

    appendField(lines, QStringLiteral("Status"), signatureStatusText(signature.status));
    appendField(lines, QStringLiteral("Signed"), signature.is_signed);
    appendField(lines, QStringLiteral("Profile found"), signature.profile_found);
    appendField(lines, QStringLiteral("Certificate found"), signature.certificate_found);
    appendField(lines, QStringLiteral("Certificate parsed"), signature.certificate_parsed);
    appendField(lines, QStringLiteral("AppGallery issued"), signature.app_gallery_issued);
    appendField(lines, QStringLiteral("Version name"), fromUtf8(signature.version_name));
    appendField(lines, QStringLiteral("Issuer"), fromUtf8(signature.issuer));
    appendField(lines, QStringLiteral("Certificate status"), fromUtf8(signature.certificate_status));
    appendField(lines, QStringLiteral("Certificate status message"), fromUtf8(signature.certificate_status_message));
    appendField(lines, QStringLiteral("Certificate valid"), signature.certificate_valid);
    appendField(lines, QStringLiteral("Signer"), fromUtf8(signature.signer));
    appendField(lines, QStringLiteral("Certificate issuer"), fromUtf8(signature.certificate_issuer));
    appendField(lines, QStringLiteral("Valid from"), fromUtf8(signature.valid_from));
    appendField(lines, QStringLiteral("Valid to"), fromUtf8(signature.valid_to));
    appendField(lines, QStringLiteral("Algorithm"), fromUtf8(signature.algorithm));
    appendField(lines, QStringLiteral("Key size"), signature.key_size);
    appendField(lines, QStringLiteral("Thumbprint"), fromUtf8(signature.thumbprint));

    if (!signature.fingerprints.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Fingerprints"));
        lines.append(QStringLiteral("------------"));
        for (const auto& fingerprint : signature.fingerprints) {
            lines.append(QStringLiteral("- %1").arg(fromUtf8(fingerprint)));
        }
    }

    if (!signature.certificates.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Certificates"));
        lines.append(QStringLiteral("------------"));
        for (int i = 0; i < static_cast<int>(signature.certificates.size()); ++i) {
            if (i > 0) {
                lines.append(QString());
            }
            lines.append(formatCertificate(signature.certificates.at(static_cast<std::size_t>(i)), i));
        }
    }

    if (!signature.diagnostics.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Diagnostics"));
        lines.append(QStringLiteral("-----------"));
        for (const auto& diagnostic : signature.diagnostics) {
            lines.append(QStringLiteral("- %1").arg(fromUtf8(diagnostic)));
        }
    }

    if (!signature.development_certificate_pem.empty()) {
        lines.append(QString());
        lines.append(QStringLiteral("Development certificate PEM"));
        lines.append(QStringLiteral("---------------------------"));
        lines.append(fromUtf8(signature.development_certificate_pem));
    }

    return normalizeSourceContent(lines.join(QLatin1Char('\n')));
}

DecompiledSourceFile inspectSignatureFile(const std::string& hylePath)
{
    auto signature = hyle::hap::inspect_hap_signature(hylePath);
    if (!signature) {
        return {
            QStringLiteral("Signature.txt"),
            QStringLiteral("TXT"),
            errorMessage("Inspect signature failed", signature.error()),
            QStringLiteral("signature"),
            QStringLiteral("text"),
            0,
            false
        };
    }

    return {
        QStringLiteral("Signature.txt"),
        QStringLiteral("TXT"),
        formatSignatureInfo(*signature),
        QStringLiteral("signature"),
        QStringLiteral("text"),
        0,
        false
    };
}

DecompiledSourceFile inspectSummaryFile(const hyle::hap::decompiled_package_session& session)
{
    auto summary = session.summary();
    if (!summary) {
        return {
            QStringLiteral("Summary"),
            QStringLiteral("TXT"),
            errorMessage("Summarize package failed", summary.error()),
            QStringLiteral("summary"),
            QStringLiteral("text"),
            0,
            false
        };
    }

    return {
        QStringLiteral("Summary"),
        QStringLiteral("TXT"),
        normalizeSourceContent(fromUtf8(hyle::hap::format_decompiled_package_summary(*summary))),
        QStringLiteral("summary"),
        QStringLiteral("text"),
        0,
        false
    };
}

QString normalizeSourceContent(QString content)
{
    content.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    content.replace(QLatin1Char('\r'), QLatin1Char('\n'));

    QStringList lines = content.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    while (!lines.isEmpty() && lines.last().trimmed().isEmpty()) {
        lines.removeLast();
    }
    return lines.join(QLatin1Char('\n'));
}

std::vector<DecompiledSourceFile> fromSourceFiles(
    const std::vector<hyle::hap::decompiled_source_file>& sourceFiles)
{
    std::vector<DecompiledSourceFile> files;
    files.reserve(sourceFiles.size());

    for (const auto& file : sourceFiles) {
        files.push_back({
            fromUtf8(file.path),
            languageKind(file.language),
            normalizeSourceContent(fromUtf8(file.content)),
            QStringLiteral("source"),
            QStringLiteral("text"),
            0,
            false
        });
    }

    return files;
}

} // namespace

namespace HyleDecompiler {

OpenResult openFile(const QString& filePath)
{
    OpenResult result;
    const QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        result.error = QObject::tr("File does not exist: %1").arg(filePath);
        return result;
    }

    const auto hylePath = toUtf8Path(filePath);

    if (isAbcFile(filePath)) {
        auto files = hyle::hap::decompile_abc_sources(hylePath);
        if (!files) {
            result.error = errorMessage("Decompile failed", files.error());
            return result;
        }
        result.files = fromSourceFiles(*files);
        result.status = QObject::tr("Decompiled %1 file(s)").arg(static_cast<int>(result.files.size()));
        return result;
    }

    auto session = hyle::hap::open_decompiled_package(hylePath);
    if (!session) {
        result.error = errorMessage("Open package failed", session.error());
        return result;
    }

    const auto sourceFileCount = session->source_files().size();
    result.files.reserve(sourceFileCount + session->resources().size() + 2U);
    for (const auto& file : session->source_files()) {
        result.files.push_back({
            fromUtf8(file.path),
            languageKind(file.language),
            {},
            QStringLiteral("source"),
            QStringLiteral("text"),
            file.id,
            true
        });
    }
    for (const auto& resource : session->resources()) {
        result.files.push_back({
            fromUtf8(resource.path),
            resourceKindName(resource.kind),
            {},
            QStringLiteral("resource"),
            isImageResource(resource.kind, resource.path)
                ? QStringLiteral("image")
                : isTextResource(resource.kind) ? QStringLiteral("text") : QStringLiteral("hex"),
            resource.id,
            !resource.is_directory,
            resource.is_directory
        });
    }

    result.files.push_back(inspectSignatureFile(hylePath));
    result.files.push_back(inspectSummaryFile(*session));

    result.session = std::make_shared<Session>(std::move(*session));
    result.sessionMutex = std::make_shared<std::mutex>();
    result.status = QObject::tr("Loaded %1 source file(s)").arg(static_cast<int>(sourceFileCount));
    return result;
}

SourceResult readResourceContent(
    const std::shared_ptr<Session>& session,
    const std::shared_ptr<std::mutex>& sessionMutex,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name)
{
    SourceResult result;
    result.nodeIndex = nodeIndex;
    result.name = name;

    if (!session || !sessionMutex) {
        result.error = QObject::tr("Decompiler session is not available.");
        return result;
    }

    std::lock_guard<std::mutex> lock(*sessionMutex);
    auto content = session->read_resource(hyleId);
    if (!content) {
        result.error = errorMessage("Read resource failed", content.error());
        return result;
    }

    result.kind = resourceKindName(content->kind);
    if (isImageResource(*content)) {
        result.contentMode = QStringLiteral("image");
        result.content = formatImageResource(*content);
    } else if (isTextResource(content->kind)) {
        result.contentMode = QStringLiteral("text");
        result.content = normalizeSourceContent(bytesToUtf8Text(content->bytes));
    } else {
        result.contentMode = QStringLiteral("hex");
        result.content = formatHexResource(*content);
    }
    return result;
}

SourceResult decompileSourceFile(
    const std::shared_ptr<Session>& session,
    const std::shared_ptr<std::mutex>& sessionMutex,
    int nodeIndex,
    std::size_t hyleId,
    const QString& name)
{
    SourceResult result;
    result.nodeIndex = nodeIndex;
    result.name = name;

    if (!session || !sessionMutex) {
        result.error = QObject::tr("Decompiler session is not available.");
        return result;
    }

    std::lock_guard<std::mutex> lock(*sessionMutex);
    auto package = session->decompile_source_file(hyleId);
    if (!package) {
        result.error = errorMessage("Decompile failed", package.error());
        return result;
    }

    for (const auto& diagnostic : package->diagnostics) {
        if (!result.diagnostics.isEmpty()) {
            result.diagnostics += QLatin1Char('\n');
        }
        result.diagnostics += fromUtf8(diagnostic.source);
        result.diagnostics += QStringLiteral(": ");
        result.diagnostics += fromUtf8(diagnostic.message);
    }

    if (!package->files.empty()) {
        const auto& source = package->files.front();
        result.content = normalizeSourceContent(fromUtf8(source.content));
        result.kind = languageKind(source.language);
    } else {
        result.content = QObject::tr("// Hyle returned no source content.");
    }

    return result;
}

} // namespace HyleDecompiler
