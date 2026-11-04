// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/MarkdownBuilder.h"
#include "core/MathRenderer.h"
#include "viewer/MathObjectHandler.h"

#include <QColor>
#include <QDir>
#include <QElapsedTimer>
#include <QImage>
#include <QPainter>
#include <QRegularExpression>
#include <QTextCursor>
#include <QTextDocument>

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
    void rendersTheSummationOperator();
    void rendersBigDelimiters();
    void blankRenderFallsBackToNoImage();
    void rendersAGalleryOfFormulas();
    void glyphsStayInsideTheirLayoutBox();
    void handlerDrawsTheFormula();
    void handlerFallsBackToSourceText();
    void handlerTypesetsAtTheTextSize();
    void staysFastForManyFormulas();

private:
    static bool hasInk(const QImage &image);
    static int inkColumnCount(const QImage &image);
    static int firstInkColumn(const QImage &image);
    static int lastInkColumn(const QImage &image);
    static int inkHeight(const QImage &image);
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

void MathTest::rendersTheSummationOperator()
{
    MathRenderer &renderer = MathRenderer::instance();
    renderer.clearCache();

    // The display-size summation comes from the size-variant slots of cmex10,
    // which the font backend used to lay out but draw as a hole.
    const QImage image = renderer.image(QStringLiteral("\\sum"), true, 16, QColor(Qt::black), 1.0);
    QVERIFY2(!image.isNull() && hasInk(image), "the summation operator was not drawn");
}

void MathTest::rendersBigDelimiters()
{
    MathRenderer &renderer = MathRenderer::instance();
    renderer.clearCache();

    // A delimiter grows to its content, and only the grown variants come from
    // the size-variant slots that used to render as holes.
    const QImage bare = renderer.image(QStringLiteral("\\frac{a}{b}"), false, 16, QColor(Qt::black), 1.0);
    const QImage delimited = renderer.image(QStringLiteral("\\left( \\frac{a}{b} \\right)"), false, 16, QColor(Qt::black), 1.0);
    QVERIFY(!bare.isNull() && !delimited.isNull());

    // The delimited formula reserves room for its delimiters: when those glyphs
    // fail to draw, its ink is exactly the bare fraction's. Working delimiters
    // add ink columns of their own on both flanks.
    QVERIFY2(inkColumnCount(delimited) >= inkColumnCount(bare) + 4, "the delimiters were laid out but not drawn");
}

int MathTest::inkColumnCount(const QImage &image)
{
    int columns = 0;
    for (int x = 0; x < image.width(); ++x) {
        for (int y = 0; y < image.height(); ++y) {
            if (qAlpha(image.pixel(x, y)) > 0) {
                ++columns;
                break;
            }
        }
    }
    return columns;
}

int MathTest::firstInkColumn(const QImage &image)
{
    for (int x = 0; x < image.width(); ++x)
        for (int y = 0; y < image.height(); ++y)
            if (qAlpha(image.pixel(x, y)) > 0)
                return x;
    return image.width();
}

int MathTest::lastInkColumn(const QImage &image)
{
    for (int x = image.width() - 1; x >= 0; --x)
        for (int y = 0; y < image.height(); ++y)
            if (qAlpha(image.pixel(x, y)) > 0)
                return x;
    return -1;
}

int MathTest::inkHeight(const QImage &image)
{
    // Both images in this comparison are drawn black on white, so a colour test
    // is what works for either (the formula bitmap is otherwise transparent).
    const QRgb white = QColor(Qt::white).rgb();
    int top = image.height();
    int bottom = -1;
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            if (image.pixel(x, y) != white) {
                top = qMin(top, y);
                bottom = qMax(bottom, y);
                break;
            }
    return bottom < top ? 0 : bottom - top + 1;
}

void MathTest::blankRenderFallsBackToNoImage()
{
    MathRenderer &renderer = MathRenderer::instance();
    renderer.clearCache();

    // \hspace parses and has layout width, but produces no ink. A blank bitmap
    // is exactly how a glyph the backend failed to draw presents itself, so the
    // renderer refuses it and the pane falls back to the formula's source text.
    const QImage image = renderer.image(QStringLiteral("\\hspace{2em}"), false, 16, QColor(Qt::black), 1.0);
    QVERIFY2(image.isNull(), "a blank bitmap was returned as if it were a render");
}

void MathTest::rendersAGalleryOfFormulas()
{
    // One test per glyph class that has broken before. Every entry must produce
    // a bitmap with actual ink, and none may fall back to "no image".
    MathRenderer &renderer = MathRenderer::instance();
    renderer.clearCache();

    const QStringList gallery = {
        QStringLiteral("E = mc^2"),
        QStringLiteral("e^{i\\pi} + 1 = 0"),
        QStringLiteral("\\int_0^{\\infty} e^{-x^2}\\,dx = \\frac{\\sqrt{\\pi}}{2} \\sum_{k=1}^{n} k^2 = \\frac{n(n+1)(2n+1)}{6}"),
        QStringLiteral("\\sum_{i=1}^{n} x_i"),
        QStringLiteral("\\prod_{k=1}^{n} a_k"),
        QStringLiteral("\\int_0^{\\infty} e^{-x}\\,dx"),
        QStringLiteral("\\lim_{x \\to 0} \\frac{\\sin x}{x}"),
        QStringLiteral("\\sqrt[3]{x + 1}"),
        QStringLiteral("\\left( \\frac{a}{b} \\right)^2"),
        QStringLiteral("\\begin{pmatrix} a & b \\\\ c & d \\end{pmatrix}"),
        QStringLiteral("\\alpha \\beta \\gamma \\Omega \\phi"),
        QStringLiteral("f(x) = \\frac{1}{\\sigma\\sqrt{2\\pi}} e^{-\\frac{x^2}{2}}"),
    };

    for (const QString &formula : gallery) {
        const QImage image = renderer.image(formula, true, 16, QColor(Qt::black), 1.0);
        QVERIFY2(!image.isNull(), qPrintable(QStringLiteral("no image for %1").arg(formula)));
        QVERIFY2(hasInk(image), qPrintable(QStringLiteral("nothing drawn for %1").arg(formula)));
    }
}

void MathTest::handlerDrawsTheFormula()
{
    MathObjectHandler handler;
    QFont body;
    body.setPointSizeF(12);
    handler.setBodyFont(body);
    handler.setColor(QColor(Qt::black));
    handler.setDevicePixelRatio(1.0);

    QTextDocument document;
    QTextCharFormat format;
    format.setObjectType(MarkdownBuilder::mathObjectType);
    format.setProperty(MarkdownBuilder::mathSourceKey, QStringLiteral("x^2 + y^2"));
    format.setProperty(MarkdownBuilder::mathDisplayKey, false);
    QTextCursor(&document).insertText(QString(QChar::ObjectReplacementCharacter), format);

    const QSizeF size = handler.intrinsicSize(&document, 0, format);
    QVERIFY2(size.width() > 8 && size.height() > 8, "the formula has no measurable size");

    QImage image(int(size.width()) + 8, int(size.height()) + 8, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    handler.drawObject(&painter, QRectF(4, 4, size.width(), size.height()), &document, 0, format);
    painter.end();

    // Ink that is not the white backdrop: the formula itself.
    bool drawn = false;
    for (int y = 0; y < image.height() && !drawn; ++y)
        for (int x = 0; x < image.width() && !drawn; ++x)
            if (image.pixel(x, y) != QColor(Qt::white).rgb())
                drawn = true;
    QVERIFY2(drawn, "drawObject produced a blank area");
}

void MathTest::handlerFallsBackToSourceText()
{
    // \hspace has layout width but no ink, so the renderer refuses it: the
    // handler must still show something readable -- the source text.
    MathObjectHandler handler;
    QFont body;
    body.setPointSizeF(12);
    handler.setBodyFont(body);
    handler.setColor(QColor(Qt::black));
    handler.setDevicePixelRatio(1.0);

    QTextDocument document;
    QTextCharFormat format;
    format.setObjectType(MarkdownBuilder::mathObjectType);
    format.setProperty(MarkdownBuilder::mathSourceKey, QStringLiteral("\\hspace{2em}"));
    format.setProperty(MarkdownBuilder::mathDisplayKey, false);
    QTextCursor(&document).insertText(QString(QChar::ObjectReplacementCharacter), format);

    const QSizeF size = handler.intrinsicSize(&document, 0, format);
    QVERIFY2(size.width() > 8, "even the fallback needs a measurable size");

    QImage image(int(size.width()) + 8, int(size.height()) + 8, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    handler.drawObject(&painter, QRectF(4, 4, size.width(), size.height()), &document, 0, format);
    painter.end();

    bool drawn = false;
    for (int y = 0; y < image.height() && !drawn; ++y)
        for (int x = 0; x < image.width() && !drawn; ++x)
            if (image.pixel(x, y) != QColor(Qt::white).rgb())
                drawn = true;
    QVERIFY2(drawn, "the refused formula was drawn as nothing at all");
}

void MathTest::glyphsStayInsideTheirLayoutBox()
{
    // A glyph drawn wider than its layout slot lands on its neighbour, and the
    // overshoot accumulates along the formula: "abcdef" used to overflow its
    // layout box on the right. The bitmap reserves kPadding on each side; only
    // the italic overhang of Computer Modern may reach past the advance width,
    // by about a pixel.
    MathRenderer &renderer = MathRenderer::instance();
    renderer.clearCache();

    const QStringList formulas = {
        QStringLiteral("E = mc^2"),
        QStringLiteral("abcdef"),
        QStringLiteral("\\int_0^{\\infty} e^{-x^2}\\,dx = \\frac{\\sqrt{\\pi}}{2}"),
    };

    for (const QString &formula : formulas) {
        const QImage image = renderer.image(formula, true, 16, QColor(Qt::black), 1.0);
        QVERIFY(!image.isNull());
        QVERIFY2(firstInkColumn(image) >= 3, qPrintable(QStringLiteral("ink overflows the left edge: %1").arg(formula)));
        QVERIFY2(lastInkColumn(image) <= image.width() - 2, qPrintable(QStringLiteral("ink overflows the right edge: %1").arg(formula)));
    }
}

void MathTest::handlerTypesetsAtTheTextSize()
{
    // A formula must be typeset at the size of the surrounding text: it is the
    // object's whole job to blend in. Comparing a lone "H" from the formula
    // with the same letter from the text font, on the same device, catches a
    // formula set at the wrong em size without hard-coding any font metric.
    MathObjectHandler handler;
    QFont body;
    body.setPointSizeF(12);
    handler.setBodyFont(body);
    // What MarkdownView passes: the body font's em in device-independent
    // pixels, the unit the document layout measures in.
    handler.setEmSize(body.pointSizeF() * 96.0 / 72.0);
    handler.setColor(QColor(Qt::black));
    handler.setDevicePixelRatio(1.0);

    QTextDocument document;
    QTextCharFormat format;
    format.setObjectType(MarkdownBuilder::mathObjectType);
    format.setProperty(MarkdownBuilder::mathSourceKey, QStringLiteral("H"));
    format.setProperty(MarkdownBuilder::mathDisplayKey, false);
    QTextCursor(&document).insertText(QString(QChar::ObjectReplacementCharacter), format);

    const QSizeF size = handler.intrinsicSize(&document, 0, format);
    QVERIFY(size.height() > 0);

    QImage image(int(size.width()) + 4, int(size.height()) + 4, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    handler.drawObject(&painter, QRectF(2, 2, size.width(), size.height()), &document, 0, format);
    painter.end();

    // Reference: the same letter drawn by the text font. Inter and Computer
    // Modern have slightly different cap-height ratios, so the two agree within
    // about a tenth when the sizes match -- and are a fifth apart when the
    // formula is set at two thirds of the text size.
    QImage reference(80, 60, QImage::Format_ARGB32_Premultiplied);
    reference.fill(Qt::white);
    {
        QPainter referencePainter(&reference);
        referencePainter.setFont(body);
        referencePainter.drawText(QPointF(10, 35), QStringLiteral("H"));
    }

    const int formulaInk = inkHeight(image);
    const int textInk = inkHeight(reference);
    QVERIFY(formulaInk > 0 && textInk > 0);
    QVERIFY2(qAbs(formulaInk - textInk) <= qRound(textInk * 0.12),
             qPrintable(QStringLiteral("formula is %1px tall, the text is %2px").arg(formulaInk).arg(textInk)));
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
