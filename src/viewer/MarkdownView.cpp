// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/MarkdownView.h"

#include "core/FontManager.h"
#include "viewer/CodeHighlighter.h"
#include "viewer/GifObjectHandler.h"

#include <QAbstractTextDocumentLayout>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>

MarkdownView::MarkdownView(QWidget *parent)
    : QTextBrowser(parent)
    , m_gifHandler(new GifObjectHandler(document(), this))
    , m_codeHighlighter(new CodeHighlighter)
{
    setOpenExternalLinks(true);
    setOpenLinks(true);
    setFrameShape(QFrame::NoFrame);

    document()->documentLayout()->registerHandler(MarkdownBuilder::gifObjectType, m_gifHandler);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    connect(m_renderTimer, &QTimer::timeout, this, &MarkdownView::rebuild);

    m_style.fonts = FontManager();
    m_codeHighlighter->setThemeDefinition(m_style.theme);
}

void MarkdownView::setRenderStyle(const RenderStyle &style)
{
    m_style = style;
    m_gifHandler->setMaximumWidth(style.maxImageWidth);
    m_codeHighlighter->setThemeDefinition(style.theme);
    setPalette(style.theme.palette);
    renderNow();
}

const RenderStyle &MarkdownView::renderStyle() const
{
    return m_style;
}

void MarkdownView::setBaseDir(const QString &directory)
{
    if (m_style.baseDir == directory)
        return;
    m_style.baseDir = directory;
    renderNow();
}

void MarkdownView::setMarkdown(const QString &markdown)
{
    m_source = markdown;
    if (m_renderTimer->interval() == 0) {
        rebuild();
    } else if (!m_renderTimer->isActive()) {
        // Coalesce: the first change arms the timer, later ones are absorbed.
        m_renderTimer->start();
    }
}

void MarkdownView::renderNow()
{
    m_renderTimer->stop();
    rebuild();
}

int MarkdownView::renderInterval() const
{
    return m_renderTimer->interval();
}

void MarkdownView::setRenderInterval(int milliseconds)
{
    m_renderTimer->setInterval(milliseconds);
}

void MarkdownView::rebuild()
{
    QScrollBar *scrollBar = verticalScrollBar();
    const int previousValue = scrollBar->value();
    const QString anchorText = cursorForPosition(QPoint(0, 0)).block().text();

    MarkdownBuilder::build(document(), m_source, m_style);
    m_codeHighlighter->highlight(document());

    // Prefer to keep the anchor block visually in place; fall back to the
    // absolute scroll offset when it cannot be found (e.g. the block was
    // edited away).
    bool anchored = false;
    if (!anchorText.isEmpty()) {
        for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
            if (block.text() != anchorText)
                continue;
            QTextCursor cursor(document());
            cursor.setPosition(block.position());
            const int top = cursorRect(cursor).top();
            if (top != 0)
                scrollBar->setValue(scrollBar->value() + top);
            anchored = true;
            break;
        }
    }
    if (!anchored)
        scrollBar->setValue(previousValue);

    emit rendered();
}