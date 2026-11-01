// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/MarkdownBuilder.h"
#include "viewer/GifObjectHandler.h"

#include <QFile>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QTextCursor>
#include <QTextDocument>

// A two-frame 4x4 GIF (red, then blue), generated once and embedded so the
// test has no external tooling dependency.
static const char *kAnimatedGifBase64 =
    "R0lGODlhBAAEAIEAAP8AAAAAAAAAAAAAACH/"
    "C05FVFNDQVBFMi4wAwEAAAAh+QQACgAAACwAAAAABAAEAAAICQABCBxIsCCAgAAh+QQBCgABACwAAAAABAAEAIEAAP8AAAAAAAAAAAAICQABCBxIsCCAgAA7";

class GifTest : public QObject
{
    Q_OBJECT

private slots:
    void handlerMeasuresIntrinsicSize();
    void handlerCapsWidthToMaximum();
    void animationAdvancesFrames();
    void staleAnimationsAreDropped();

private:
    QTextCharFormat objectFormat(const QString &path) const;
    static QImage render(GifObjectHandler *handler, QTextDocument *document, const QTextCharFormat &format, const QSizeF &size);
};

QTextCharFormat GifTest::objectFormat(const QString &path) const
{
    QTextCharFormat format;
    format.setObjectType(MarkdownBuilder::gifObjectType);
    format.setProperty(MarkdownBuilder::gifPathKey, path);
    return format;
}

QImage GifTest::render(GifObjectHandler *handler, QTextDocument *document, const QTextCharFormat &format, const QSizeF &size)
{
    QImage image(size.toSize(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    handler->drawObject(&painter, QRectF(QPointF(0, 0), size), document, 0, format);
    painter.end();
    return image;
}

void GifTest::handlerMeasuresIntrinsicSize()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("anim.gif"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromBase64(kAnimatedGifBase64));
    file.close();

    QTextDocument document;
    GifObjectHandler handler(&document);
    const QSizeF size = handler.intrinsicSize(&document, 0, objectFormat(path));
    QCOMPARE(size, QSizeF(4, 4));
}

void GifTest::handlerCapsWidthToMaximum()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("anim.gif"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromBase64(kAnimatedGifBase64));
    file.close();

    QTextDocument document;
    GifObjectHandler handler(&document);
    handler.setMaximumWidth(2);
    const QSizeF size = handler.intrinsicSize(&document, 0, objectFormat(path));
    QCOMPARE(size, QSizeF(2, 2)); // scaled down keeping the aspect ratio
}

void GifTest::animationAdvancesFrames()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("anim.gif"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromBase64(kAnimatedGifBase64));
    file.close();

    QTextDocument document;
    GifObjectHandler handler(&document);
    const QTextCharFormat format = objectFormat(path);
    const QSizeF size = handler.intrinsicSize(&document, 0, format);
    QVERIFY(size.isValid());
    QVERIFY(handler.hasRunningAnimations());

    const QImage first = render(&handler, &document, format, size);

    // Poll until a later frame is drawn; a fixed wait could land back on the
    // first frame of a two-frame loop.
    QImage second = first;
    for (int attempt = 0; attempt < 60 && second == first; ++attempt) {
        QTest::qWait(20);
        second = render(&handler, &document, format, size);
    }
    QVERIFY(first != second);
}

void GifTest::staleAnimationsAreDropped()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("anim.gif"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QByteArray::fromBase64(kAnimatedGifBase64));
    file.close();

    QTextDocument document;
    GifObjectHandler handler(&document);
    const QTextCharFormat format = objectFormat(path);

    QTextCursor cursor(&document);
    cursor.insertText(QString(QChar::ObjectReplacementCharacter), format);
    QVERIFY(handler.intrinsicSize(&document, 0, format).isValid());
    QVERIFY(handler.hasRunningAnimations());

    // The image disappears from the document: the movie must be dropped.
    document.clear();
    handler.syncToDocument();
    QVERIFY(!handler.hasRunningAnimations());
}

QTEST_MAIN(GifTest)
#include "test_gif.moc"