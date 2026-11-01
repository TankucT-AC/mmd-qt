// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/FontManager.h"
#include "core/MarkdownBuilder.h"
#include "core/Theme.h"

#include <QDir>
#include <QFile>
#include <QFontInfo>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QTextList>
#include <QTextTable>
#include <QTextTableCell>
#include <QUrl>

class MarkdownTest : public QObject
{
    Q_OBJECT

private slots:
    void headingLevelAndScale();
    void headingSixUsesWeakColor();
    void codeBlockIsOneContinuousRegion();
    void wrappedCodeLineStaysInsideTheBox();
    void htmlDivCentersContent();
    void htmlSummaryIsRendered();
    void htmlInlineTagsAreApplied();
    void htmlKbdAndRemoteImageBecomeChips();
    void calloutIsRendered();
    void headingSlugsMatchGithubStyle();
    void htmlImageHonoursWidthAttribute();
    void readmeRendersWithoutLiteralHtml();
    void boldEmphasisInParagraph();
    void inlineCodeSpan();
    void fencedCodeBlockCarriesLanguage();
    void bulletList();
    void orderedList();
    void taskList();
    void blockQuote();
    void linkSpan();
    void strikethroughSpan();
    void gfmTable();
    void imageResolvesAgainstBaseDir();
    void animatedImageBecomesObject();
    void svgIsRasterisedWithAntialiasing();
    void horizontalRule();
    void entityDecoding();
    void softBreakStaysOneParagraph();
    void emptyDocument();
    void missingImageShowsPlaceholder();

private:
    void render(QTextDocument *document, const QString &markdown, const QString &baseDir = QString());
    static QTextCharFormat formatAt(const QTextBlock &block, int position);
    static QImage renderImage(QTextDocument *document, int width);
    static int codeLeftEdge(const QImage &image, int row, QRgb code);
    static const QColor &codeColor();
};

const QColor &MarkdownTest::codeColor()
{
    static const QColor color = ThemeDefinition::light().markdown.codeBackground;
    return color;
}

QImage MarkdownTest::renderImage(QTextDocument *document, int width)
{
    document->setTextWidth(width);
    const int height = int(document->size().height()) + 2;
    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    document->drawContents(&painter, QRectF(0, 0, width, height));
    painter.end();
    return image;
}

int MarkdownTest::codeLeftEdge(const QImage &image, int row, QRgb code)
{
    for (int x = 0; x < image.width(); ++x) {
        if (image.pixel(x, row) == code)
            return x;
    }
    return -1;
}

void MarkdownTest::render(QTextDocument *document, const QString &markdown, const QString &baseDir)
{
    RenderStyle style;
    style.theme = ThemeDefinition::light();
    style.baseDir = baseDir;

    QVERIFY(MarkdownBuilder::build(document, markdown, style));
}

QTextCharFormat MarkdownTest::formatAt(const QTextBlock &block, int position)
{
    const int absolute = block.position() + position;
    for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
        const QTextFragment fragment = it.fragment();
        if (fragment.contains(absolute))
            return fragment.charFormat();
    }
    return QTextCharFormat();
}

void MarkdownTest::headingLevelAndScale()
{
    QTextDocument document;
    render(&document, QStringLiteral("# Title\n"));
    const QTextBlock block = document.firstBlock();
    QCOMPARE(block.blockFormat().intProperty(MarkdownBuilder::kindKey), int(MarkdownBuilder::BlockKind::Heading));
    QCOMPARE(block.blockFormat().intProperty(MarkdownBuilder::headingLevelKey), 1);
    QCOMPARE(block.text(), QStringLiteral("Title"));

    const qreal bodySize = FontManager().bodyFont().pointSizeF();
    QVERIFY(block.charFormat().fontPointSize() > bodySize);
    QVERIFY(block.charFormat().font().bold());
}

void MarkdownTest::headingSixUsesWeakColor()
{
    const ThemeDefinition theme = ThemeDefinition::light();
    QTextDocument document;
    render(&document, QStringLiteral("###### Small\n"));
    const QTextBlock block = document.firstBlock();
    QCOMPARE(block.blockFormat().intProperty(MarkdownBuilder::headingLevelKey), 6);
    QCOMPARE(block.charFormat().foreground().color(), theme.markdown.weakText);
}

void MarkdownTest::boldEmphasisInParagraph()
{
    QTextDocument document;
    render(&document, QStringLiteral("plain **bold** text\n"));
    const QTextBlock block = document.firstBlock();
    QCOMPARE(block.text(), QStringLiteral("plain bold text"));

    QTextCharFormat boldFormat;
    boldFormat.setFontWeight(QFont::Bold);
    QVERIFY(formatAt(block, int(block.text().indexOf(QStringLiteral("bold")))).fontWeight() == QFont::Bold);
    QVERIFY(formatAt(block, 0).fontWeight() == QFont::Normal);
}

void MarkdownTest::codeBlockIsOneContinuousRegion()
{
    QTextDocument document;
    render(&document, QStringLiteral("```\nline1\n\nline2\n```\n"));
    document.setTextWidth(600);

    const int height = int(document.size().height()) + 2;
    QImage image(600, height, QImage::Format_ARGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    document.drawContents(&painter, QRectF(0, 0, 600, height));
    painter.end();

    const QRgb code = ThemeDefinition::light().markdown.codeBackground.rgb();

    int first = -1;
    int last = -1;
    for (int y = 0; y < height; ++y) {
        if (image.pixel(300, y) == code) {
            if (first < 0)
                first = y;
            last = y;
        }
    }
    QVERIFY(first >= 0);

    // No unshaded row between the first and last row of the block: a blank
    // line inside the fence must not split it into separate boxes.
    for (int y = first; y <= last; ++y)
        QCOMPARE(image.pixel(300, y), code);

    // The shaded region spans (almost) the whole content width.
    int left = -1;
    int right = -1;
    for (int x = 0; x < 600; ++x) {
        if (image.pixel(x, (first + last) / 2) == code) {
            if (left < 0)
                left = x;
            right = x;
        }
    }
    QVERIFY(right - left > 400);
}

void MarkdownTest::wrappedCodeLineStaysInsideTheBox()
{
    QTextDocument document;
    render(&document, QStringLiteral("```\nQElapsedTimer m_frameTimer; // a very long comment that must wrap somewhere across lines\nshort;\n```\n"));

    // Narrow width forces the long line to wrap.
    const QImage image = renderImage(&document, 320);
    const QRgb code = codeColor().rgb();

    int first = -1;
    int last = -1;
    for (int y = 0; y < image.height(); ++y) {
        if (image.pixel(image.width() / 2, y) == code) {
            if (first < 0)
                first = y;
            last = y;
        }
    }
    QVERIFY(first >= 0);

    // The shaded box must be a rectangle: its left edge is the same on every
    // row, so a wrapped continuation line never spills out to the left.
    const int edge = codeLeftEdge(image, first, code);
    for (int y = first; y <= last; ++y)
        QCOMPARE(codeLeftEdge(image, y, code), edge);
}

void MarkdownTest::inlineCodeSpan()
{
    QTextDocument document;
    render(&document, QStringLiteral("a `code` b\n"));
    const QTextBlock block = document.firstBlock();
    const int at = int(block.text().indexOf(QStringLiteral("code")));

    const QTextCharFormat format = formatAt(block, at);
    QCOMPARE(format.font().family(), QStringLiteral("JetBrains Mono"));
    QCOMPARE(format.background().color(), ThemeDefinition::light().markdown.codeBackground);
}

void MarkdownTest::fencedCodeBlockCarriesLanguage()
{
    QTextDocument document;
    render(&document, QStringLiteral("```cpp\nint x = 0;\n```\n"));

    QTextBlock block;
    for (QTextBlock candidate = document.firstBlock(); candidate.isValid(); candidate = candidate.next()) {
        if (candidate.blockFormat().intProperty(MarkdownBuilder::kindKey) == int(MarkdownBuilder::BlockKind::Code)) {
            block = candidate;
            break;
        }
    }
    QVERIFY(block.isValid());

    QCOMPARE(block.blockFormat().intProperty(MarkdownBuilder::kindKey), int(MarkdownBuilder::BlockKind::Code));
    QCOMPARE(block.blockFormat().stringProperty(MarkdownBuilder::languageKey), QStringLiteral("cpp"));
    QCOMPARE(block.text(), QStringLiteral("int x = 0;"));
    QVERIFY(QFontInfo(block.charFormat().font()).fixedPitch());
}

void MarkdownTest::bulletList()
{
    QTextDocument document;
    render(&document, QStringLiteral("- one\n- two\n"));
    QTextList *list = nullptr;
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        if (QTextList *candidate = block.textList()) {
            list = candidate;
            break;
        }
    }
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 2);
    QCOMPARE(list->format().style(), QTextListFormat::ListDisc);
}

void MarkdownTest::orderedList()
{
    QTextDocument document;
    render(&document, QStringLiteral("1. one\n2. two\n"));
    QTextList *list = nullptr;
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        if (QTextList *candidate = block.textList()) {
            list = candidate;
            break;
        }
    }
    QVERIFY(list != nullptr);
    QCOMPARE(list->count(), 2);
    QCOMPARE(list->format().style(), QTextListFormat::ListDecimal);
}

void MarkdownTest::taskList()
{
    QTextDocument document;
    render(&document, QStringLiteral("- [ ] todo\n- [x] done\n"));

    QString todo;
    QString done;
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        if (block.text().contains(QStringLiteral("todo")))
            todo = block.text();
        if (block.text().contains(QStringLiteral("done")))
            done = block.text();
    }
    // The marker is a checkbox object, so the leading glyph is the object
    // replacement character; the label text follows it.
    QVERIFY(todo.contains(QStringLiteral("todo")));
    QVERIFY(done.contains(QStringLiteral("done")));

    // Checked and unchecked items carry the checkbox object type; the checked
    // one has its "checked" property set.
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat format = it.fragment().charFormat();
            if (format.objectType() != MarkdownBuilder::checkboxObjectType)
                continue;
            const bool checked = format.property(MarkdownBuilder::checkboxCheckedKey).toBool();
            if (block.text().contains(QStringLiteral("done")))
                QVERIFY(checked);
            if (block.text().contains(QStringLiteral("todo")))
                QVERIFY(!checked);
        }
        // Done items must NOT be struck through any more.
        const int at = int(block.text().indexOf(QStringLiteral("done")));
        if (at >= 0)
            QVERIFY(!formatAt(block, at).fontStrikeOut());
    }
}

void MarkdownTest::blockQuote()
{
    QTextDocument document;
    render(&document, QStringLiteral("> quoted\n"));

    // The quote is a one-cell table with a left bar; find it and check the bar.
    bool found = false;
    for (QTextFrame *frame : document.rootFrame()->childFrames()) {
        auto *table = qobject_cast<QTextTable *>(frame);
        if (!table)
            continue;
        if (table->cellAt(0, 0).firstCursorPosition().block().text() != QStringLiteral("quoted"))
            continue;
        const QTextTableCellFormat cellFormat = table->cellAt(0, 0).format().toTableCellFormat();
        QVERIFY(cellFormat.leftBorder() > 0.0);
        QVERIFY(cellFormat.leftPadding() > 0.0);
        found = true;
    }
    QVERIFY(found);
}

void MarkdownTest::linkSpan()
{
    const ThemeDefinition theme = ThemeDefinition::light();
    QTextDocument document;
    render(&document, QStringLiteral("[site](https://example.com)\n"));
    const QTextBlock block = document.firstBlock();
    const int at = int(block.text().indexOf(QStringLiteral("site")));

    bool found = false;
    for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
        const QTextFragment fragment = it.fragment();
        if (fragment.contains(at)) {
            const QTextCharFormat format = fragment.charFormat();
            QVERIFY(format.isAnchor());
            QCOMPARE(format.anchorHref(), QStringLiteral("https://example.com"));
            QCOMPARE(format.foreground().color(), theme.markdown.link);
            found = true;
        }
    }
    QVERIFY(found);
}

void MarkdownTest::strikethroughSpan()
{
    QTextDocument document;
    render(&document, QStringLiteral("~~gone~~\n"));
    const QTextBlock block = document.firstBlock();
    const int at = int(block.text().indexOf(QStringLiteral("gone")));
    QVERIFY(formatAt(block, at).fontStrikeOut());
}

void MarkdownTest::gfmTable()
{
    QTextDocument document;
    render(&document, QStringLiteral("| a | b |\n| --- | --- |\n| 1 | 2 |\n"));
    QTextTable *table = nullptr;
    for (QTextFrame *frame : document.rootFrame()->childFrames()) {
        if (QTextTable *candidate = qobject_cast<QTextTable *>(frame)) {
            table = candidate;
            break;
        }
    }
    QVERIFY(table != nullptr);
    QCOMPARE(table->rows(), 2);
    QCOMPARE(table->columns(), 2);
    QCOMPARE(table->cellAt(0, 0).firstCursorPosition().block().text(), QStringLiteral("a"));
    QCOMPARE(table->cellAt(1, 1).firstCursorPosition().block().text(), QStringLiteral("2"));
}

void MarkdownTest::imageResolvesAgainstBaseDir()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile png(dir.filePath(QStringLiteral("img.png")));
    QVERIFY(png.open(QIODevice::WriteOnly));
    png.write(QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=="));
    png.close();

    QTextDocument document;
    render(&document, QStringLiteral("![alt](img.png)\n"), dir.path());
    QTextImageFormat image;
    bool found = false;
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat format = it.fragment().charFormat();
            if (format.isImageFormat()) {
                image = format.toImageFormat();
                found = true;
            }
        }
    }
    QVERIFY(found);
    QCOMPARE(image.name(), QFileInfo(dir.filePath(QStringLiteral("img.png"))).absoluteFilePath());
}

void MarkdownTest::animatedImageBecomesObject()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile gif(dir.filePath(QStringLiteral("anim.gif")));
    QVERIFY(gif.open(QIODevice::WriteOnly));
    // Minimal single-frame GIF87a header; enough for suffix-based detection.
    gif.write(QByteArray::fromBase64("R0lGODlhAQABAIAAAAAAAP///yH5BAEAAAAALAAAAAABAAEAAAIBRAA7"));
    gif.close();

    QTextDocument document;
    render(&document, QStringLiteral("![alt](anim.gif)\n"), dir.path());
    bool found = false;
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat format = it.fragment().charFormat();
            if (format.objectType() == MarkdownBuilder::gifObjectType)
                found = true;
        }
    }
    QVERIFY(found);
}

void MarkdownTest::svgIsRasterisedWithAntialiasing()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile svg(dir.filePath(QStringLiteral("circle.svg")));
    QVERIFY(svg.open(QIODevice::WriteOnly));
    svg.write(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"48\" height=\"48\">"
        "<circle cx=\"24\" cy=\"24\" r=\"18\" fill=\"#ff0000\"/></svg>");
    svg.close();

    QTextDocument document;
    render(&document, QStringLiteral("![circle](circle.svg)\n"), dir.path());

    QTextImageFormat image;
    bool found = false;
    for (QTextBlock block = document.firstBlock(); block.isValid() && !found; block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat format = it.fragment().charFormat();
            if (format.isImageFormat()) {
                image = format.toImageFormat();
                found = true;
            }
        }
    }
    QVERIFY(found);
    // The SVG is handed over as a document resource.
    QVERIFY(image.name().startsWith(QStringLiteral("mmd-res-")));
    const QVariant resource = document.resource(QTextDocument::ImageResource, QUrl(image.name()));
    QVERIFY(resource.isValid());
    const QImage rasterised = resource.value<QImage>();
    QVERIFY(!rasterised.isNull());

    // Antialiasing leaves partially transparent pixels along the circle's edge;
    // without it every pixel would be fully opaque or fully transparent.
    int edgePixels = 0;
    for (int y = 0; y < rasterised.height(); ++y) {
        for (int x = 0; x < rasterised.width(); ++x) {
            const int alpha = qAlpha(rasterised.pixel(x, y));
            if (alpha > 0 && alpha < 255)
                ++edgePixels;
        }
    }
    QVERIFY(edgePixels > 0);
}

void MarkdownTest::horizontalRule()
{
    const ThemeDefinition theme = ThemeDefinition::light();
    QTextDocument document;
    render(&document, QStringLiteral("above\n\n---\n\nbelow\n"));
    bool found = false;
    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        if (block.blockFormat().intProperty(MarkdownBuilder::kindKey) == int(MarkdownBuilder::BlockKind::Rule)) {
            QVERIFY(!block.text().isEmpty());
            QCOMPARE(block.charFormat().foreground().color(), theme.markdown.rule);
            QCOMPARE(block.blockFormat().alignment(), Qt::AlignHCenter);
            found = true;
        }
    }
    QVERIFY(found);
}

void MarkdownTest::entityDecoding()
{
    QTextDocument document;
    render(&document, QStringLiteral("a &amp; b &copy; c\n"));
    const QString text = document.firstBlock().text();
    QVERIFY(text.contains(QStringLiteral("a & b")));
    QVERIFY(text.contains(QChar(0xa9)));
}

void MarkdownTest::softBreakStaysOneParagraph()
{
    QTextDocument document;
    render(&document, QStringLiteral("first line\nsecond line\n"));
    QCOMPARE(document.firstBlock().text(), QStringLiteral("first line second line"));
}

void MarkdownTest::emptyDocument()
{
    QTextDocument document;
    render(&document, QStringLiteral(""));
    QVERIFY(document.isEmpty());
}

void MarkdownTest::missingImageShowsPlaceholder()
{
    QTextDocument document;
    render(&document, QStringLiteral("![alt](missing.png)\n"), QStringLiteral("/nonexistent"));
    const QString all = document.toPlainText();
    QVERIFY(all.contains(QStringLiteral("missing.png")));
}

void MarkdownTest::htmlDivCentersContent()
{
    QTextDocument document;
    render(&document, QStringLiteral("<div align=\"center\">\n\n# Title\n\n</div>\n"));

    QTextBlock block;
    for (QTextBlock candidate = document.firstBlock(); candidate.isValid(); candidate = candidate.next()) {
        if (candidate.text() == QStringLiteral("Title"))
            block = candidate;
    }
    QVERIFY(block.isValid());
    QCOMPARE(block.blockFormat().alignment(), Qt::AlignHCenter);
}

void MarkdownTest::htmlSummaryIsRendered()
{
    QTextDocument document;
    render(&document, QStringLiteral("<details>\n<summary><strong>Table of contents</strong></summary>\n\n- one\n\n</details>\n"));

    const QString text = document.toPlainText();
    QVERIFY(text.contains(QStringLiteral("Table of contents")));
    QVERIFY(text.contains(QString(QChar(0x25B8)))); // the ▸ marker
    QVERIFY(!text.contains(QStringLiteral("<summary")));
}

void MarkdownTest::htmlInlineTagsAreApplied()
{
    QTextDocument document;
    render(&document, QStringLiteral("x <b>BOLD</b> y <code>CODE</code> z <sub>SUB</sub>\n"));

    const QTextBlock block = document.firstBlock();
    QVERIFY(block.isValid());
    QCOMPARE(formatAt(block, int(block.text().indexOf(QStringLiteral("BOLD")))).fontWeight(), int(QFont::Bold));
    QCOMPARE(formatAt(block, int(block.text().indexOf(QStringLiteral("CODE")))).background().color(), ThemeDefinition::light().markdown.codeBackground);
    QCOMPARE(formatAt(block, int(block.text().indexOf(QStringLiteral("SUB")))).verticalAlignment(), QTextCharFormat::AlignSubScript);
}

void MarkdownTest::htmlKbdAndRemoteImageBecomeChips()
{
    auto chipTexts = [](QTextDocument &doc) {
        QStringList out;
        for (QTextBlock block = doc.firstBlock(); block.isValid(); block = block.next()) {
            for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
                const QTextCharFormat format = it.fragment().charFormat();
                if (format.objectType() == MarkdownBuilder::chipObjectType)
                    out << format.property(MarkdownBuilder::chipTextKey).toString();
            }
        }
        return out;
    };

    QTextDocument keyboard;
    render(&keyboard, QStringLiteral("press <kbd>Ctrl</kbd> now\n"));
    QCOMPARE(chipTexts(keyboard), QStringList{QStringLiteral("Ctrl")});

    QTextDocument badge;
    render(&badge, QStringLiteral("x <img src=\"https://img.shields.io/badge.svg\" alt=\"build: passing\"> y\n"));
    QCOMPARE(chipTexts(badge), QStringList{QStringLiteral("build: passing")});
}

void MarkdownTest::calloutIsRendered()
{
    QTextDocument document;
    render(&document, QStringLiteral("> [!NOTE]\n> body text\n"));

    bool found = false;
    for (QTextFrame *frame : document.rootFrame()->childFrames()) {
        auto *table = qobject_cast<QTextTable *>(frame);
        if (!table)
            continue;
        const QTextTableCell cell = table->cellAt(0, 0);
        if (cell.firstCursorPosition().block().text() != QStringLiteral("Note"))
            continue;
        QCOMPARE(cell.format().toTableCellFormat().leftBorderBrush().color(), ThemeDefinition::light().markdown.link);
        found = true;
    }
    QVERIFY(found);
}

void MarkdownTest::headingSlugsMatchGithubStyle()
{
    QCOMPARE(MarkdownBuilder::headingSlug(QStringLiteral("Key features")), QStringLiteral("key-features"));
    QCOMPARE(MarkdownBuilder::headingSlug(QStringLiteral("Getting started")), QStringLiteral("getting-started"));
    QCOMPARE(MarkdownBuilder::headingSlug(QStringLiteral("Licensing rules (mandatory)")), QStringLiteral("licensing-rules-mandatory"));
    QCOMPARE(MarkdownBuilder::headingSlug(QStringLiteral("C++20 & Qt")), QStringLiteral("c20--qt"));
}

void MarkdownTest::htmlImageHonoursWidthAttribute()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QImage image(200, 40, QImage::Format_ARGB32);
    image.fill(Qt::red);
    QVERIFY(image.save(dir.filePath(QStringLiteral("pic.png"))));

    QTextDocument document;
    render(&document, QStringLiteral("<img src=\"pic.png\" alt=\"x\" width=\"60\">\n"), dir.path());

    bool found = false;
    for (QTextBlock block = document.firstBlock(); block.isValid() && !found; block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat format = it.fragment().charFormat();
            if (!format.isImageFormat())
                continue;
            const QTextImageFormat imageFormat = format.toImageFormat();
            QCOMPARE(int(imageFormat.width()), 60);
            QCOMPARE(int(imageFormat.height()), 12); // 200x40 scaled to width 60
            found = true;
        }
    }
    QVERIFY(found);
}

void MarkdownTest::readmeRendersWithoutLiteralHtml()
{
    QFile file(QStringLiteral(MMD_SOURCE_DIR "/README.md"));
    QVERIFY2(file.open(QIODevice::ReadOnly), qPrintable(file.errorString()));

    QTextDocument document;
    render(&document, QString::fromUtf8(file.readAll()), QStringLiteral(MMD_SOURCE_DIR));

    const QString text = document.toPlainText();
    QVERIFY(text.contains(QStringLiteral("mmd-qt")));
    QVERIFY(text.contains(QStringLiteral("\u25B8 Table of contents")));
    // The hero's raw markup must be gone (the README documents the tags as
    // inline code, so check the actual markup, not the tag names).
    QVERIFY(!text.contains(QStringLiteral("<div")));
    QVERIFY(!text.contains(QStringLiteral("<summary><strong>")));
    QVERIFY(!text.contains(QStringLiteral("<img")));
}

QTEST_MAIN(MarkdownTest)
#include "test_markdown.moc"
