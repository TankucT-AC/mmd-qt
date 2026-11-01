// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "editor/MarkdownEditor.h"

#include "core/FontManager.h"
#include "editor/MarkdownHighlighter.h"

#include <QPainter>
#include <QTextBlock>

#include <algorithm>

namespace
{

constexpr int kLineNumberPadding = 8;

}

class MarkdownEditor::LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(MarkdownEditor *editor)
        : QWidget(editor)
        , m_editor(editor)
    {
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    MarkdownEditor *m_editor; // not owned
};

MarkdownEditor::MarkdownEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_highlighter(new MarkdownHighlighter(document()))
    , m_lineNumberArea(new LineNumberArea(this))
{
    setFont(FontManager().codeFont());
    setLineWrapMode(QPlainTextEdit::NoWrap);
    setFrameShape(QFrame::NoFrame);

    connect(this, &QPlainTextEdit::blockCountChanged, this, &MarkdownEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest, this, &MarkdownEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, &MarkdownEditor::highlightCurrentLine);

    updateLineNumberAreaWidth();
    highlightCurrentLine();
}

void MarkdownEditor::setTheme(const ThemeDefinition &theme)
{
    m_theme = theme;
    m_highlighter->setTheme(theme); // re-highlights
    setPalette(theme.palette);
    highlightCurrentLine();
}

void MarkdownEditor::setDocumentLimits(const DocumentLimits &limits)
{
    setHighlightingEnabled(limits.editorHighlightEnabled());
}

void MarkdownEditor::setHighlightingEnabled(bool enabled)
{
    if (m_highlightingEnabled == enabled)
        return;

    m_highlightingEnabled = enabled;
    // Detaching stops the incremental per-block passes QSyntaxHighlighter runs
    // while the user types. Colours already applied go away with the next
    // setMarkdown(), which replaces the whole document.
    m_highlighter->setDocument(enabled ? document() : nullptr);
    if (enabled)
        m_highlighter->rehighlight(); // Qt defers this; the text is on screen now
}

void MarkdownEditor::setMarkdown(const QString &markdown)
{
    setPlainText(markdown);
    m_highlighter->rehighlight();
}

int MarkdownEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    for (int maximum = (std::max)(1, blockCount()); maximum >= 10; maximum /= 10)
        ++digits;
    return kLineNumberPadding + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void MarkdownEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);

    const QRect contents = contentsRect();
    m_lineNumberArea->setGeometry(contents.left(), contents.top(), lineNumberAreaWidth(), contents.height());
}

void MarkdownEditor::updateLineNumberAreaWidth()
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void MarkdownEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth();
}

void MarkdownEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> selections;

    QTextEdit::ExtraSelection currentLine;
    currentLine.format.setBackground(m_theme.markdown.codeBackground);
    currentLine.format.setProperty(QTextFormat::FullWidthSelection, true);
    currentLine.cursor = textCursor();
    currentLine.cursor.clearSelection();
    selections.append(currentLine);

    setExtraSelections(selections);
}

void MarkdownEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), m_theme.palette.color(QPalette::Window));

    painter.setPen(m_theme.markdown.weakText);

    const QTextBlock first = firstVisibleBlock();
    QTextBlock block = first;
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top())
            painter
                .drawText(0, top, m_lineNumberArea->width() - kLineNumberPadding / 2, fontMetrics().height(), Qt::AlignRight, QString::number(blockNumber + 1));

        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
