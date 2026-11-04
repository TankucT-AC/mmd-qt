// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "TestHttpServer.h"
#include "core/ImageFetcher.h"

#include <QImage>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <memory>

// A 1x1 transparent PNG.
static const char *kPngBase64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg==";
static const char *kSvgSource =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"20\" height=\"10\">"
    "<rect width=\"20\" height=\"10\" fill=\"#f00\"/></svg>";

class ImageFetcherTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        QVERIFY(m_server.isListening());
    }

    void fetchesPngOverHttp();
    void fetchesSvgAndRespectsDevicePixelRatio();
    void sniffsImageBytesWhenContentTypeLies();
    void failsOnMissingFile();
    void failsOnNonImageData();
    void failsOnOversizeReply();
    void timesOutOnDeadConnection();
    void followsSafeRedirect();
    void refusesRedirectIntoPrivateSpace();
    void givesUpAfterTooManyRedirects();
    void blocksLoopbackUnlessPrivateNetworkIsAllowed();
    void doesNotRepeatFailedUrls();
    void clearFailedAllowsRetry();
    void disabledFetcherTouchesNothing();
    void secondInstanceServesFromDiskCache();
    void coalescesDuplicateRequests();
    void destroyingWhileInFlightIsSafe();

private:
    std::unique_ptr<ImageFetcher> makeFetcher(const QTemporaryDir &cache)
    {
        auto fetcher = std::make_unique<ImageFetcher>();
        fetcher->setCacheDirectory(cache.path());
        fetcher->setRequestTimeoutMs(2000);
        // The fixture serves from loopback, which the SSRF guard rightly
        // refuses by default; positive-path tests opt out explicitly.
        fetcher->setPrivateNetworkAllowed(true);
        return fetcher;
    }

    TestHttpServer m_server;
};

void ImageFetcherTest::fetchesPngOverHttp()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/logo.png"), {200, {{"Content-Type", "image/png"}}, QByteArray::fromBase64(kPngBase64), true});

    auto fetcher = makeFetcher(cache);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/logo.png")));
    QVERIFY(ready.wait(10000));
    QCOMPARE(failed.count(), 0);
    QCOMPARE(m_server.hits(QStringLiteral("/logo.png")), 1);

    const QImage image = ready.at(0).at(1).value<QImage>();
    QCOMPARE(image.width(), 1);
    QCOMPARE(image.height(), 1);
}

void ImageFetcherTest::fetchesSvgAndRespectsDevicePixelRatio()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/pic.svg"), {200, {{"Content-Type", "image/svg+xml"}}, QByteArray(kSvgSource), true});

    auto fetcher = makeFetcher(cache);
    fetcher->setDevicePixelRatio(2.0);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);

    fetcher->fetch(m_server.url(QStringLiteral("/pic.svg")));
    QVERIFY(ready.wait(10000));

    const QImage image = ready.at(0).at(1).value<QImage>();
    QCOMPARE(image.width(), 40); // 20 x dpr 2
    QCOMPARE(image.height(), 20);
    // Tagged with the ratio, so the document draws it at the logical 20x10
    // while keeping the extra pixels.
    QCOMPARE(image.devicePixelRatio(), 2.0);
    QCOMPARE(image.deviceIndependentSize().toSize(), QSize(20, 10));
}

void ImageFetcherTest::sniffsImageBytesWhenContentTypeLies()
{
    QTemporaryDir cache;
    // raw.githubusercontent serves SVG with a text content type; the bytes,
    // not the header, decide.
    m_server.setRoute(QStringLiteral("/relabeled.png"), {200, {{"Content-Type", "text/plain"}}, QByteArray::fromBase64(kPngBase64), true});

    auto fetcher = makeFetcher(cache);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/relabeled.png")));
    QVERIFY(ready.wait(10000));
    QCOMPARE(failed.count(), 0);
}

void ImageFetcherTest::failsOnMissingFile()
{
    QTemporaryDir cache;
    auto fetcher = makeFetcher(cache);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/definitely-missing.png")));
    QVERIFY(failed.wait(10000));
    QCOMPARE(ready.count(), 0);
}

void ImageFetcherTest::failsOnNonImageData()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/junk.png"), {200, {{"Content-Type", "text/plain"}}, QByteArrayLiteral("this is not an image at all"), true});

    auto fetcher = makeFetcher(cache);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/junk.png")));
    QVERIFY(failed.wait(10000));
    QCOMPARE(ready.count(), 0);
}

void ImageFetcherTest::failsOnOversizeReply()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/huge.png"), {200, {{"Content-Type", "image/png"}}, QByteArray(2000, 'x'), true});

    auto fetcher = makeFetcher(cache);
    fetcher->setMaximumBytes(1000);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/huge.png")));
    QVERIFY(failed.wait(10000));
    QCOMPARE(ready.count(), 0);
}

void ImageFetcherTest::timesOutOnDeadConnection()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/hang.png"), {200, {}, QByteArray(), false});

    auto fetcher = makeFetcher(cache);
    fetcher->setRequestTimeoutMs(300);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/hang.png")));
    failed.wait(3000);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(ready.count(), 0);
}

void ImageFetcherTest::followsSafeRedirect()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/hop"), {302, {{"Location", "/hop-dest.png"}}, QByteArray(), true});
    m_server.setRoute(QStringLiteral("/hop-dest.png"), {200, {{"Content-Type", "image/png"}}, QByteArray::fromBase64(kPngBase64), true});

    auto fetcher = makeFetcher(cache);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/hop")));
    QVERIFY(ready.wait(10000));
    QCOMPARE(failed.count(), 0);
    QCOMPARE(m_server.hits(QStringLiteral("/hop-dest.png")), 1);
}

void ImageFetcherTest::refusesRedirectIntoPrivateSpace()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/hop-out"), {302, {{"Location", "http://10.255.255.9/secret.png"}}, QByteArray(), true});

    auto fetcher = makeFetcher(cache); // private network allowed, still: the
    // redirect target must be re-checked against the same rules as the origin
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/hop-out")));
    failed.wait(10000);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(ready.count(), 0);
}

void ImageFetcherTest::givesUpAfterTooManyRedirects()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/loop"), {302, {{"Location", "/loop"}}, QByteArray(), true});

    auto fetcher = makeFetcher(cache);
    fetcher->setMaximumRedirects(2);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/loop")));
    failed.wait(10000);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(ready.count(), 0);
    QVERIFY(m_server.hits(QStringLiteral("/loop")) <= 3); // origin + 2 follow-ups
}

void ImageFetcherTest::blocksLoopbackUnlessPrivateNetworkIsAllowed()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/blocked.png"), {200, {{"Content-Type", "image/png"}}, QByteArray::fromBase64(kPngBase64), true});

    auto fetcher = makeFetcher(cache);
    fetcher->setPrivateNetworkAllowed(false); // the point of the test
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/blocked.png")));
    failed.wait(3000);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(ready.count(), 0);
    QCOMPARE(m_server.hits(QStringLiteral("/blocked.png")), 0); // never reached the wire
}

void ImageFetcherTest::doesNotRepeatFailedUrls()
{
    QTemporaryDir cache;
    auto fetcher = makeFetcher(cache);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/definitely-missing.png")));
    QVERIFY(failed.wait(10000));
    QCOMPARE(failed.count(), 1);

    // Relayouts ask again; the fetcher must not turn one missing file into a
    // request storm.
    fetcher->fetch(m_server.url(QStringLiteral("/definitely-missing.png")));
    QCOMPARE(failed.count(), 1);
}

void ImageFetcherTest::clearFailedAllowsRetry()
{
    QTemporaryDir cache;
    auto fetcher = makeFetcher(cache);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    // No route yet: the first attempt fails with a 404.
    fetcher->fetch(m_server.url(QStringLiteral("/flaky.png")));
    QVERIFY(failed.wait(10000));

    // The network "comes back" (the route exists now) and clearFailed() lets
    // the retry through.
    m_server.setRoute(QStringLiteral("/flaky.png"), {200, {{"Content-Type", "image/png"}}, QByteArray::fromBase64(kPngBase64), true});
    fetcher->clearFailed();
    fetcher->fetch(m_server.url(QStringLiteral("/flaky.png")));
    QVERIFY(ready.wait(10000));
    QCOMPARE(ready.count(), 1);
}

void ImageFetcherTest::disabledFetcherTouchesNothing()
{
    QTemporaryDir cache;
    auto fetcher = makeFetcher(cache);
    fetcher->setEnabled(false);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);
    QSignalSpy failed(fetcher.get(), &ImageFetcher::failed);

    fetcher->fetch(m_server.url(QStringLiteral("/disabled.png")));
    QTest::qWait(200);

    QCOMPARE(ready.count(), 0);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(m_server.hits(QStringLiteral("/disabled.png")), 0);
}

void ImageFetcherTest::secondInstanceServesFromDiskCache()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/cached.png"), {200, {{"Content-Type", "image/png"}}, QByteArray::fromBase64(kPngBase64), true});

    auto first = makeFetcher(cache);
    QSignalSpy ready(first.get(), &ImageFetcher::ready);
    first->fetch(m_server.url(QStringLiteral("/cached.png")));
    QVERIFY(ready.wait(10000));
    QCOMPARE(m_server.hits(QStringLiteral("/cached.png")), 1);

    // A brand-new fetcher (empty memory cache) must answer from the disk
    // cache without touching the network again -- synchronously, so a layout
    // query never has to wait.
    auto second = makeFetcher(cache);
    QSignalSpy ready2(second.get(), &ImageFetcher::ready);
    second->fetch(m_server.url(QStringLiteral("/cached.png")));
    QCOMPARE(ready2.count(), 1);
    QCOMPARE(m_server.hits(QStringLiteral("/cached.png")), 1);
    QVERIFY(!second->cachedImage(m_server.url(QStringLiteral("/cached.png"))).isNull());
}

void ImageFetcherTest::coalescesDuplicateRequests()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/once.png"), {200, {{"Content-Type", "image/png"}}, QByteArray::fromBase64(kPngBase64), true});

    auto fetcher = makeFetcher(cache);
    QSignalSpy ready(fetcher.get(), &ImageFetcher::ready);

    const QUrl url = m_server.url(QStringLiteral("/once.png"));
    fetcher->fetch(url);
    fetcher->fetch(url);
    QVERIFY(fetcher->isFetching(url));

    QVERIFY(ready.wait(10000));
    QTest::qWait(200); // late duplicates would emit here
    QCOMPARE(ready.count(), 1);
    QCOMPARE(m_server.hits(QStringLiteral("/once.png")), 1);
}

void ImageFetcherTest::destroyingWhileInFlightIsSafe()
{
    QTemporaryDir cache;
    m_server.setRoute(QStringLiteral("/never.png"), {200, {}, QByteArray(), false}); // hangs

    {
        auto fetcher = makeFetcher(cache);
        fetcher->fetch(m_server.url(QStringLiteral("/never.png")));
        QTest::qWait(50); // let the request go out, then destroy mid-flight
    }

    // Reaching here without a crash is the assertion: tearing the fetcher down
    // while a reply is pending must not re-enter fail() and emit into a
    // half-destroyed owner (which used to abort).
    QTest::qWait(50);
    QVERIFY(true);
}

QTEST_MAIN(ImageFetcherTest)
#include "test_imagefetcher.moc"