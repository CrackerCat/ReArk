#ifndef REARK_RESOURCE_PREVIEW_PROVIDER_H
#define REARK_RESOURCE_PREVIEW_PROVIDER_H

#include <QByteArray>
#include <QHash>
#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>
#include <QString>
#include <QStringList>

class ResourcePreviewProvider : public QQuickImageProvider {
public:
    ResourcePreviewProvider();

    [[nodiscard]] QString storeImage(const QByteArray& bytes);
    [[nodiscard]] QString storeMediaFile(const QString& sourceName, const QByteArray& bytes);
    void clear();

    [[nodiscard]] QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

private:
    [[nodiscard]] QString cacheDirectory() const;

    QHash<QString, QByteArray> images_;
    QStringList cachedFiles_;
    mutable QMutex mutex_;
};

#endif // REARK_RESOURCE_PREVIEW_PROVIDER_H
