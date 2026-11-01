// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "editor/MarkdownEditor.h"
#include "editor/MarkdownHighlighter.h"

#include <QTextBlock>
#include <QTextDocument>

class EditorTest : public QObject
{
    Q_OBJECT

private slots:
    void headingLineHighlighted();
    void fenceBlocksGetCodeStyleAndState();
    void inlineCodeSpanHighlighted();
    void linkColored();
    void editorAppliesThemeAndCountsLines();
    void themeSwitchRehighlights();

private:
    static QTextCharFormat formatAt(const QTextBlock &block, int position);
    static QTextDocument *makeDocument(const QString &markdown, MarkdownHighlighter **highlighter);
};

QTextCharFormat EditorTest::formatAt(const QTextBlock &block, int position)
{
    // QSyntaxHighlighter formats live in the block layout, not in fragments.
    const auto ranges = block.layout()->formats();
    for (const QTextLayout::FormatRange &range : ranges) {
        if (position >= range.start && position < range.start + range.length)
            return range.format;
    }
    for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
        if (it.fragment().contains(block.position() + position))
            return it.fragment().charFormat();
    }
    return QTextCharFormat();
}

QTextDocument *EditorTest::makeDocument(const QString &markdown, MarkdownHighlighter **highlighter)
{
    // The text must be set before the theme: see MarkdownEditor::setMarkdown
    // for why programmatic replacement needs an explicit rehighlight.
    auto *document = new QTextDocument;
    auto *hl = new MarkdownHighlighter(document);
    document->setPlainText(markdown);
    hl->setTheme(ThemeDefinition::light());
    *highlighter = hl;
    return document;
}

void EditorTest::headingLineHighlighted()
{
    const ThemeDefinition theme = ThemeDefinition::light();

    MarkdownHighlighter *highlighter = nullptr;
    std::unique_ptr<QTextDocument> document(makeDocument(QStringLiteral("# Title\n"), &highlighter));
    Q_UNUSED(highlighter);

    const QTextBlock block = document->firstBlock();
    QCOMPARE(formatAt(block, 0).fontWeight(), int(QFont::Bold));
    QCOMPARE(formatAt(block, 0).foreground().color(), theme.markdown.heading);
}

void EditorTest::fenceBlocksGetCodeStyleAndState()
{
    const ThemeDefinition theme = ThemeDefinition::light();

    MarkdownHighlighter *highlighter = nullptr;
    std::unique_ptr<QTextDocument> document(makeDocument(QStringLiteral("before\n```cpp\nint a;\n```\nafter\n"), &highlighter));
    Q_UNUSED(highlighter);

    QVector<QTextBlock> blocks;
    for (QTextBlock block = document->firstBlock(); block.isValid(); block = block.next())
        blocks.append(block);

    QCOMPARE(blocks.size(), 6); // five source lines plus the trailing empty block
    // Fence line and content are styled as code.
    QCOMPARE(formatAt(blocks[1], 0).background().color(), theme.markdown.codeBackground);
    QCOMPARE(formatAt(blocks[2], 0).background().color(), theme.markdown.codeBackground);
    // State machine closed the fence: "after" is plain again.
    QVERIFY(!formatAt(blocks[4], 0).hasProperty(QTextFormat::BackgroundBrush));
}

void EditorTest::inlineCodeSpanHighlighted()
{
    const ThemeDefinition theme = ThemeDefinition::light();

    MarkdownHighlighter *highlighter = nullptr;
    std::unique_ptr<QTextDocument> document(makeDocument(QStringLiteral("a `code` b\n"), &highlighter));
    Q_UNUSED(highlighter);

    const QTextBlock block = document->firstBlock();
    const QTextCharFormat format = formatAt(block, block.text().indexOf(QStringLiteral("code")));
    QCOMPARE(format.background().color(), theme.markdown.codeBackground);
    QCOMPARE(format.foreground().color(), theme.markdown.codeText);
}

void EditorTest::linkColored()
{
    const ThemeDefinition theme = ThemeDefinition::light();

    MarkdownHighlighter *highlighter = nullptr;
    std::unique_ptr<QTextDocument> document(makeDocument(QStringLiteral("see [docs](https://example.com) now\n"), &highlighter));
    Q_UNUSED(highlighter);

    const QTextBlock block = document->firstBlock();
    const QTextCharFormat format = formatAt(block, block.text().indexOf(QStringLiteral("docs")));
    QCOMPARE(format.foreground().color(), theme.markdown.link);
}

void EditorTest::editorAppliesThemeAndCountsLines()
{
    MarkdownEditor editor;
    editor.setMarkdown(QStringLiteral("one\ntwo\n"));
    QCOMPARE(editor.document()->blockCount(), 3); // trailing block after the last newline
    QVERIFY(editor.lineNumberAreaWidth() > 0);

    editor.setTheme(ThemeDefinition::dark());
    QCOMPARE(editor.document()->blockCount(), 3);
}

void EditorTest::themeSwitchRehighlights()
{
    MarkdownHighlighter *highlighter = nullptr;
    std::unique_ptr<QTextDocument> document(makeDocument(QStringLiteral("# Title\n"), &highlighter));

    const QColor before = formatAt(document->firstBlock(), 0).foreground().color();
    highlighter->setTheme(ThemeDefinition::dark());
    const QColor after = formatAt(document->firstBlock(), 0).foreground().color();
    QVERIFY(before != after);
}

QTEST_MAIN(EditorTest)
#include "test_editor.moc"