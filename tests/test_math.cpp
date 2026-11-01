// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/MathRenderer.h"

#include <QColor>
#include <QDir>
#include <QElapsedTimer>
#include <QImage>

class MathTest : public QObject
{
    Q_OBJECT

private slots:
    void findsTheTexResources();
    void rendersAnInlineFormula();
    void rendersADisplayFormula();
    void cachesRepeatedFormulas();
    void rejectsEmptySources();
    void survivesNonsenseInput();
    void staysFastForManyFormulas();

private:
    static bool hasInk(const QImage &image);
};

bool MathTest::hasInk(const QImage &image)
{
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (qAlpha(image.pixel(x, y)) > 0)
                return true;
        }
    }
    return false;
}

void MathTest::findsTheTexResources()
{
    const QString directory = MathRenderer::resourceDirectory();
    QVERIFY2(!directory.isEmpty(), "TeX resources not found; MicroTeX cannot render anything");
    QVERIFY(QDir(directory + QStringLiteral("/fonts")).exists());
    QVERIFY(MathRenderer::instance().isAvailable());
}

void MathTest::rendersAnInlineFormula()
{
    const QImage image = MathRenderer::instance().image(QStringLiteral("x^2 + y^2 = z^2"), false, 16, QColor(Qt::black), 1.0);
    QVERIFY(!image.isNull());
    QVERIFY(image.width() > 20);
    QVERIFY(image.height() > 5);
    QVERIFY(hasInk(image));
}

void MathTest::rendersADisplayFormula()
{
    const QImage image = MathRenderer::instance().image(QStringLiteral("\\int_0^1 x^2\\,dx = \\frac{1}{3}"), true, 16, QColor(Qt::black), 1.0);
    QVERIFY(!image.isNull());
    // A display formula with limits and a fraction is taller than a line of text.
    QVERIFY(image.height() > 20);
    QVERIFY(hasInk(image));
}

void MathTest::cachesRepeatedFormulas()
{
    MathRenderer &renderer = MathRenderer::instance();
    renderer.clearCache();

    const QImage first = renderer.image(QStringLiteral("\\frac{1}{2}"), false, 16, QColor(Qt::black), 1.0);
    QVERIFY(!first.isNull());
    QCOMPARE(renderer.cacheSize(), 1);

    const QImage second = renderer.image(QStringLiteral("\\frac{1}{2}"), false, 16, QColor(Qt::black), 1.0);
    QCOMPARE(renderer.cacheSize(), 1);
    QCOMPARE(second.cacheKey(), first.cacheKey());

    // The colour is baked into the bitmap, so a theme switch must not reuse it.
    renderer.image(QStringLiteral("\\frac{1}{2}"), false, 16, QColor(Qt::red), 1.0);
    QCOMPARE(renderer.cacheSize(), 2);

    renderer.clearCache();
    QCOMPARE(renderer.cacheSize(), 0);
}

void MathTest::rejectsEmptySources()
{
    const QImage image = MathRenderer::instance().image(QStringLiteral("   "), false, 16, QColor(Qt::black), 1.0);
    QVERIFY(image.isNull());
}

void MathTest::survivesNonsenseInput()
{
    // Whatever the source, the pane must not crash on it.
    const QImage image = MathRenderer::instance().image(QStringLiteral("\\thisIsNotACommand{"), false, 16, QColor(Qt::black), 1.0);
    Q_UNUSED(image); // empty or not, the point is that it returned
}

void MathTest::staysFastForManyFormulas()
{
    MathRenderer &renderer = MathRenderer::instance();
    renderer.clearCache();

    QElapsedTimer timer;
    timer.start();
    for (int i = 0; i < 40; ++i) {
        const QImage image = renderer.image(QStringLiteral("f(%1) = \\sum_{k=0}^{%1} k^2").arg(i), false, 16, QColor(Qt::black), 1.0);
        QVERIFY(!image.isNull());
    }
    const qint64 elapsed = timer.elapsed();

    // A bound that would catch a pathological regression, not a benchmark.
    QVERIFY2(elapsed < 4000, qPrintable(QStringLiteral("40 formulas took %1 ms").arg(elapsed)));
}

QTEST_MAIN(MathTest)
#include "test_math.moc"
