// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/ImageFetcher.h"

#include <QBuffer>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QHostAddress>
#include <QHostInfo>
#include <QImage>
#include <QImageReader>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QVector>

namespace
{
// Responses up to this many bytes are accepted even when their content type
// does not say "image": some hosts (raw.githubusercontent among them) serve
// SVG with a text type. The bytes decide.
constexpr qsizetype kSniffBytes = 256;

bool isSvgPayload(const QString &contentType, const QByteArray &bytes)
{
    if (contentType.contains(QLatin1String("svg"), Qt::CaseInsensitive))
        return true;
    const QByteArray head = bytes.left(kSniffBytes).trimmed();
    if (head.startsWith("<?xml"))
        return head.contains("<svg");
    return head.startsWith("<svg");
}

// The SSRF guard: a hostile markdown file must not be able to make the viewer
// knock on loopback or private network space.
bool isBlockedAddress(const QHostAddress &address)
{
    if (address.isLoopback() || address.isLinkLocal() || address.isUniqueLocalUnicast() || address.isMulticast())
        return true;

    bool ok = false;
    const quint32 v4 = address.toIPv4Address(&ok);
    if (!ok)
        return false;
    return (v4 >> 24) == 0x00 // 0.0.0.0/8: "this network"
        || (v4 >> 24) == 0x0A // 10.0.0.0/8
        || (v4 >> 22) == 0x191 // 100.64.0.0/10 (CGNAT)
        || (v4 >> 20) == 0xAC1 // 172.16.0.0/12
        || (v4 >> 16) == 0xA9FE // 169.254.0.0/16 (link local)
        || (v4 >> 16) == 0xC0A8; // 192.168.0.0/16
}

bool isBlockedHost(const QString &host)
{
    if (host.compare(QLatin1String("localhost"), Qt::CaseInsensitive) == 0)
        return true;
    if (host.endsWith(QLatin1String(".local"), Qt::CaseInsensitive))
        return true;

    QHostAddress literal;
    if (literal.setAddress(host))
        return isBlockedAddress(literal);
    return false;
}
} // namespace

// One load, from the first request to the decoded image. A QObject so the
// signal lambdas can use it as their context: whatever happens to the reply,
// a deleted request takes its callbacks with it.
struct ImageFetcher::Request : QObject {
    QUrl url; // what the caller asked for; the key in the request table
    QUrl current; // the hop a redirect chain is currently on
    QNetworkReply *reply = nullptr;
    int redirectsLeft = 3;
};

ImageFetcher::ImageFetcher(QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
}

ImageFetcher::~ImageFetcher()
{
    const auto requests = m_requests;
    m_requests.clear();
    for (Request *request : requests) {
        if (request->reply) {
            request->reply->disconnect(request);
            request->reply->abort();
        }
        delete request;
    }
}

void ImageFetcher::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

bool ImageFetcher::isEnabled() const
{
    return m_enabled;
}

void ImageFetcher::setCacheDirectory(const QString &path)
{
    m_cacheDirectory = path;
}

void ImageFetcher::setRequestTimeoutMs(int ms)
{
    m_timeoutMs = ms;
}

void ImageFetcher::setMaximumBytes(qint64 bytes)
{
    m_maximumBytes = bytes;
}

void ImageFetcher::setMaximumRedirects(int redirects)
{
    m_maximumRedirects = redirects;
}

void ImageFetcher::setConcurrentRequestLimit(int limit)
{
    m_concurrentLimit = limit;
}

void ImageFetcher::setPrivateNetworkAllowed(bool allowed)
{
    m_privateAllowed = allowed;
}

void ImageFetcher::setDevicePixelRatio(qreal ratio)
{
    m_devicePixelRatio = ratio;
}

QString ImageFetcher::cacheDirectory() const
{
    if (!m_cacheDirectory.isEmpty())
        return m_cacheDirectory;
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/mmd-qt/remote-images");
}

QString ImageFetcher::diskDataPath(const QUrl &url) const
{
    const QString key = QString::fromLatin1(QCryptographicHash::hash(url.toString(QUrl::FullyEncoded).toUtf8(), QCryptographicHash::Sha256).toHex());
    return cacheDirectory() + QLatin1Char('/') + key + QStringLiteral(".data");
}

QString ImageFetcher::diskMetaPath(const QUrl &url) const
{
    return diskDataPath(url) + QStringLiteral(".meta");
}

void ImageFetcher::storeOnDisk(const QUrl &url, const QByteArray &bytes, const QString &contentType)
{
    const QString directory = cacheDirectory();
    if (!QDir().mkpath(directory))
        return;

    QFile data(diskDataPath(url));
    if (!data.open(QIODevice::WriteOnly))
        return;
    data.write(bytes);
    data.close();

    QFile meta(diskMetaPath(url));
    if (meta.open(QIODevice::WriteOnly))
        meta.write(contentType.toUtf8() + '\n' + QByteArray::number(QDateTime::currentMSecsSinceEpoch()));
}

QImage ImageFetcher::decode(const QByteArray &bytes, const QString &contentType) const
{
    if (isSvgPayload(contentType, bytes)) {
        // SVG has no pixel size until it is rendered: rasterise at the
        // intrinsic size times the ratio of the screen showing it.
        QSvgRenderer renderer(bytes);
        if (!renderer.isValid())
            return QImage();
        const QSizeF size = QSizeF(renderer.defaultSize()) * m_devicePixelRatio;
        if (size.isEmpty())
            return QImage();
        QImage image(int(size.width()), int(size.height()), QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        renderer.render(&painter, QRectF(0, 0, size.width(), size.height()));
        return image;
    }

    QBuffer buffer(const_cast<QByteArray *>(&bytes));
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer);
    reader.setAutoDetectImageFormat(true);
    return reader.read(); // GIF decodes as its first frame, played as a still
}

QImage ImageFetcher::cachedImage(const QUrl &url)
{
    const auto inMemory = m_memoryCache.constFind(url);
    if (inMemory != m_memoryCache.constEnd())
        return *inMemory;

    QFile meta(diskMetaPath(url));
    if (!meta.open(QIODevice::ReadOnly))
        return QImage();
    const QList<QByteArray> metaLines = meta.readAll().split('\n');
    meta.close();

    bool fresh = false;
    if (metaLines.size() >= 2) {
        const qint64 fetchedAt = metaLines.at(1).toLongLong();
        const qint64 age = QDateTime::currentMSecsSinceEpoch() - fetchedAt;
        fresh = age >= 0 && age < 7LL * 24 * 60 * 60 * 1000; // one week
    }
    if (!fresh) {
        QFile::remove(diskDataPath(url));
        QFile::remove(diskMetaPath(url));
        return QImage();
    }

    QFile data(diskDataPath(url));
    if (!data.open(QIODevice::ReadOnly))
        return QImage();
    const QByteArray bytes = data.readAll();
    data.close();

    const QImage image = decode(bytes, QString::fromUtf8(metaLines.at(0)));
    if (image.isNull())
        return QImage();
    m_memoryCache.insert(url, image);
    return image;
}

void ImageFetcher::fetch(const QUrl &url)
{
    if (!m_enabled || !url.isValid())
        return;

    if (!m_failed.contains(url)) {
        const QImage cached = cachedImage(url);
        if (!cached.isNull()) {
            emit ready(url, cached);
            return;
        }
    }

    // Coalesced while in flight; silently given up on once failed.
    if (m_failed.contains(url) || m_requests.contains(url))
        return;

    if (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https")) {
        fail(url);
        return;
    }

    if (m_requests.size() >= m_concurrentLimit) {
        m_pending.append(url);
        return;
    }

    Request *request = new Request;
    request->url = url;
    request->redirectsLeft = m_maximumRedirects;
    m_requests.insert(url, request);
    resolve(request, url);
}

bool ImageFetcher::isFetching(const QUrl &url) const
{
    return m_requests.contains(url) || m_pending.contains(url);
}

void ImageFetcher::clearFailed()
{
    m_failed.clear();
}

void ImageFetcher::resolve(Request *request, const QUrl &url)
{
    const QString host = url.host();
    if (host.isEmpty()) {
        fail(request->url);
        return;
    }

    // Address literals are checked on the spot; names go through the system
    // resolver and every answer is checked before we knock on anything.
    QHostAddress literal;
    if (literal.setAddress(host)) {
        if (!m_privateAllowed && isBlockedAddress(literal)) {
            fail(request->url);
            return;
        }
        startGet(request, url);
        return;
    }
    if (!m_privateAllowed && isBlockedHost(host)) {
        fail(request->url);
        return;
    }
    if (m_privateAllowed) {
        startGet(request, url);
        return;
    }

    QHostInfo::lookupHost(host, request, [this, request, url](const QHostInfo &info) {
        if (info.error() != QHostInfo::NoError) {
            fail(request->url);
            return;
        }
        for (const QHostAddress &address : info.addresses()) {
            if (isBlockedAddress(address)) {
                fail(request->url);
                return;
            }
        }
        startGet(request, url);
    });
}

void ImageFetcher::startGet(Request *request, const QUrl &url)
{
    request->current = url;
    QNetworkRequest netRequest(url);
    netRequest.setTransferTimeout(m_timeoutMs);
    netRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                            int(QNetworkRequest::ManualRedirectPolicy)); // every hop is re-validated
    request->reply = m_network->get(netRequest);

    connect(request->reply, &QNetworkReply::downloadProgress, this, [this, request](qint64 received, qint64 total) {
        if (total > m_maximumBytes || received > m_maximumBytes)
            fail(request->url);
    });
    connect(request->reply, &QNetworkReply::errorOccurred, this, [this, request](QNetworkReply::NetworkError code) {
        if (code == QNetworkReply::OperationCanceledError)
            fail(request->url); // the transfer timeout lands here
    });
    connect(request->reply, &QNetworkReply::finished, this, [this, request] {
        finish(request);
    });
}

void ImageFetcher::finish(Request *request)
{
    QNetworkReply *reply = request->reply;
    request->reply = nullptr;
    reply->deleteLater();

    // Manual redirect handling: the target travels in the reply's attributes
    // (the redirected() signal does not fire under the manual policy) and may
    // be relative to the hop it came from.
    const QUrl redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirect.isValid()) {
        if (request->redirectsLeft <= 0) {
            fail(request->url);
            return;
        }
        --request->redirectsLeft;
        resolve(request, request->current.resolved(redirect)); // re-checked like the origin
        return;
    }

    if (reply->error() != QNetworkReply::NoError) {
        fail(request->url);
        return;
    }

    const QByteArray bytes = reply->readAll();
    if (bytes.size() > m_maximumBytes) {
        fail(request->url);
        return;
    }
    const QString contentType = QString::fromLatin1(reply->header(QNetworkRequest::ContentTypeHeader).toByteArray());

    const QImage image = decode(bytes, contentType);
    const QUrl url = request->url;
    if (image.isNull()) {
        fail(url);
        return;
    }

    m_memoryCache.insert(url, image);
    storeOnDisk(url, bytes, contentType);
    m_requests.remove(url);
    request->deleteLater();
    startNextPending();
    emit ready(url, image);
}

void ImageFetcher::fail(const QUrl &url)
{
    // Mark first: abort() below re-enters fail() through errorOccurred, and
    // the failed() signal must go out exactly once.
    const bool firstTime = !m_failed.contains(url);
    m_failed.insert(url);

    if (Request *request = m_requests.take(url)) {
        if (request->reply)
            request->reply->abort();
        request->deleteLater();
    }
    m_pending.removeAll(url);
    startNextPending();

    if (firstTime)
        emit failed(url);
}

void ImageFetcher::startNextPending()
{
    while (!m_pending.isEmpty() && m_requests.size() < m_concurrentLimit) {
        const QUrl next = m_pending.takeFirst();
        if (m_failed.contains(next) || m_requests.contains(next))
            continue;
        Request *request = new Request;
        request->url = next;
        request->redirectsLeft = m_maximumRedirects;
        m_requests.insert(next, request);
        resolve(request, next);
    }
}
