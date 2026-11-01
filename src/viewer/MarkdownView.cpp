// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/MarkdownView.h"

#include "core/FontManager.h"
#include "viewer/CheckboxObjectHandler.h"
#include "viewer/CodeHighlighter.h"
#include "viewer/GifObjectHandler.h"

#include <QAbstractTextDocumentLayout>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>

namespace
{
// Coalesces bursts of edits into a single render; small enough to feel instant.
constexpr int kDefaultRenderIntervalMs = 40;
}

MarkdownView::MarkdownView(QWidget *parent)
    : QTextBrowser(parent)
    , m_gifHandler(new GifObjectHandler(document(), this))
    , m_checkboxHandler(new CheckboxObjectHandler(this))
    , m_codeHighlighter(new CodeHighlighter)
{
    setOpenExternalLinks(true);
    setOpenLinks(true);
    setFrameShape(QFrame::NoFrame);

    document()->documentLayout()->registerHandler(MarkdownBuilder::gifObjectType, m_gifHandler);
    document()->documentLayout()->registerHandler(MarkdownBuilder::checkboxObjectType, m_checkboxHandler);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(kDefaultRenderIntervalMs);
    connect(m_renderTimer, &QTimer::timeout, this, &MarkdownView::rebuild);

    m_style.fonts = FontManager();
    m_style.theme = ThemeDefinition::light();
    m_codeHighlighter->setThemeDefinition(m_style.theme);
}

MarkdownView::~MarkdownView() = default;

void MarkdownView::setRenderStyle(const RenderStyle &style)
{
    m_style = style;
    m_gifHandler->setMaximumWidth(style.maxImageWidth);
    m_checkboxHandler->setColors(style.theme.markdown.weakText, style.theme.markdown.link, style.theme.palette.color(QPalette::Base));
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

void MarkdownView::scheduleRerender()
{
    if (!m_renderTimer->isActive())
        m_renderTimer->start();
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
    const int anchorIndex = cursorForPosition(QPoint(0, 0)).block().blockNumber();

    if (!MarkdownBuilder::build(document(), m_source, m_style))
        qWarning("mmd: markdown parse failed; leaving the previous render in place");
    m_codeHighlighter->highlight(document());
    m_gifHandler->syncToDocument(); // stops movies for images no longer present

    // Prefer to keep the anchor block visually in place; fall back to the
    // absolute scroll offset when it cannot be found (e.g. the block was
    // edited away). Among equal texts, pick the occurrence closest to the
    // anchor's original index so duplicated lines do not jump the view.
    int bestPosition = -1;
    int bestDistance = 0;
    if (!anchorText.isEmpty()) {
        for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
            if (block.text() != anchorText)
                continue;
            const int distance = qAbs(block.blockNumber() - anchorIndex);
            if (bestPosition < 0 || distance < bestDistance) {
                bestPosition = block.position();
                bestDistance = distance;
            }
        }
    }

    if (bestPosition >= 0) {
        QTextCursor cursor(document());
        cursor.setPosition(bestPosition);
        const int top = cursorRect(cursor).top();
        if (top != 0)
            scrollBar->setValue(scrollBar->value() + top);
    } else {
        scrollBar->setValue(previousValue);
    }

    emit rendered();
}