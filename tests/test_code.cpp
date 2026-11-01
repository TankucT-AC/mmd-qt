// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/MarkdownBuilder.h"
#include "core/Theme.h"
#include "viewer/CodeHighlighter.h"

#include <QSet>
#include <QTextBlock>
#include <QTextDocument>

class CodeHighlightTest : public QObject
{
    Q_OBJECT

private slots:
    void codeBlockIsSplitIntoStyledFragments();
    void switchingThemeChangesCodeColors();
    void unknownLanguageIsLeftAlone();
    void plainTextIsNotTouched();

private:
    static QTextDocument *build(const QString &markdown);
    static QSet<QRgb> paletteOf(QTextDocument *document);
};

QTextDocument *CodeHighlightTest::build(const QString &markdown)
{
    RenderStyle style;
    style.theme = ThemeDefinition::light();
    auto *document = new QTextDocument;
    MarkdownBuilder::build(document, markdown, style);
    return document;
}

QSet<QRgb> CodeHighlightTest::paletteOf(QTextDocument *document)
{
    QSet<QRgb> colors;
    for (QTextBlock block = document->firstBlock(); block.isValid(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it)
            colors.insert(it.fragment().charFormat().foreground().color().rgb());
    }
    return colors;
}

void CodeHighlightTest::codeBlockIsSplitIntoStyledFragments()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("```cpp\n// comment\nint x = 0;\n```\n")));
    const QSet<QRgb> before = paletteOf(document.get());

    CodeHighlighter highlighter;
    highlighter.setThemeDefinition(ThemeDefinition::light());
    highlighter.highlight(document.get());

    const QSet<QRgb> after = paletteOf(document.get());
    QVERIFY(after.size() > before.size()); // keywords/comments got their own colors
}

void CodeHighlightTest::switchingThemeChangesCodeColors()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("```python\n# comment\nx = 1\n```\n")));

    CodeHighlighter highlighter;
    highlighter.setThemeDefinition(ThemeDefinition::light());
    highlighter.highlight(document.get());
    const QSet<QRgb> light = paletteOf(document.get());

    highlighter.setThemeDefinition(ThemeDefinition::dark());
    highlighter.highlight(document.get());
    const QSet<QRgb> dark = paletteOf(document.get());

    QVERIFY(light != dark);
}

void CodeHighlightTest::unknownLanguageIsLeftAlone()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("```definitelynotalanguage\nfoo bar\n```\n")));
    const QSet<QRgb> before = paletteOf(document.get());

    CodeHighlighter highlighter;
    highlighter.setThemeDefinition(ThemeDefinition::light());
    highlighter.highlight(document.get());

    QCOMPARE(paletteOf(document.get()), before);
}

void CodeHighlightTest::plainTextIsNotTouched()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("# Heading\n\nsome prose\n")));
    const QSet<QRgb> before = paletteOf(document.get());

    CodeHighlighter highlighter;
    highlighter.setThemeDefinition(ThemeDefinition::light());
    highlighter.highlight(document.get());

    QCOMPARE(paletteOf(document.get()), before);
}

QTEST_MAIN(CodeHighlightTest)
#include "test_code.moc"