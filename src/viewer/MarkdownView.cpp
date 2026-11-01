// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/MarkdownView.h"

#include "core/FontManager.h"
#include "viewer/CheckboxObjectHandler.h"
#include "viewer/ChipObjectHandler.h"
#include "viewer/CodeHighlighter.h"
#include "viewer/GifObjectHandler.h"
#include "viewer/MathObjectHandler.h"

#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QEvent>
#include <QGuiApplication>
#include <QLocale>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextFrame>
#include <QTimer>

#include <algorithm>
#include <functional>

namespace
{
// Coalesces bursts of edits into a single render; small enough to feel instant.
constexpr int kDefaultRenderIntervalMs = 40;

// A heading is scrolled to sit this far below the top edge, and the same slack
// decides which heading counts as current. Sharing one value keeps the two
// consistent: the section just scrolled to is always the one highlighted.
constexpr int kHeadingTopGap = 8;
}

MarkdownView::MarkdownView(QWidget *parent)
    : QTextBrowser(parent)
    , m_gifHandler(new GifObjectHandler(document(), this))
    , m_checkboxHandler(new CheckboxObjectHandler(this))
    , m_chipHandler(new ChipObjectHandler(this))
    , m_mathHandler(new MathObjectHandler(this))
    , m_codeHighlighter(new CodeHighlighter)
{
    setOpenExternalLinks(false);
    setOpenLinks(false);
    setFrameShape(QFrame::NoFrame);
    connect(this, &QTextBrowser::anchorClicked, this, &MarkdownView::handleAnchor);

    document()->documentLayout()->registerHandler(MarkdownBuilder::gifObjectType, m_gifHandler);
    document()->documentLayout()->registerHandler(MarkdownBuilder::checkboxObjectType, m_checkboxHandler);
    document()->documentLayout()->registerHandler(MarkdownBuilder::chipObjectType, m_chipHandler);
    document()->documentLayout()->registerHandler(MarkdownBuilder::mathObjectType, m_mathHandler);

    m_renderTimer = new QTimer(this);
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(kDefaultRenderIntervalMs);
    connect(m_renderTimer, &QTimer::timeout, this, &MarkdownView::rebuild);

    // Hover the code boxes to reveal their copy button.
    viewport()->setMouseTracking(true);
    m_copyFeedbackTimer = new QTimer(this);
    m_copyFeedbackTimer->setSingleShot(true);
    m_copyFeedbackTimer->setInterval(900);
    connect(m_copyFeedbackTimer, &QTimer::timeout, this, [this] {
        m_copiedFrame = nullptr;
        viewport()->update();
    });

    m_style.fonts = FontManager();
    m_style.theme = ThemeDefinition::light();
    m_codeHighlighter->setThemeDefinition(m_style.theme);
}

MarkdownView::~MarkdownView() = default;

void MarkdownView::setRenderStyle(const RenderStyle &style)
{
    m_style = style;
    m_gifHandler->setMaximumWidth(style.maxImageWidth);
    m_checkboxHandler->setColors(style.theme.markdown.weakText, style.theme.markdown.weakText, style.theme.palette.color(QPalette::Base));
    m_chipHandler->setColors(style.theme.markdown.codeBackground, style.theme.markdown.tableBorder, style.theme.markdown.weakText);
    m_mathHandler->setBodyFont(style.fonts.bodyFont());
    m_mathHandler->setColor(style.theme.palette.color(QPalette::Text));
    m_mathHandler->setDevicePixelRatio(devicePixelRatioF());
    m_codeHighlighter->setThemeDefinition(style.theme);
    setPalette(style.theme.palette);
    updateContentMargins();
    renderNow();
}

void MarkdownView::resizeEvent(QResizeEvent *event)
{
    QTextBrowser::resizeEvent(event);
    // Setting the margins inside the resize handler has no effect until the
    // next layout pass, so defer it to the event loop.
    QTimer::singleShot(0, this, [this] {
        updateContentMargins();
    });
}

void MarkdownView::scrollContentsBy(int dx, int dy)
{
    QTextBrowser::scrollContentsBy(dx, dy);
    updateVisibleHeading();
}

void MarkdownView::updateVisibleHeading()
{
    // The reading position is the top edge of the pane. Once the pane is scrolled
    // to the end of the document there is nothing left to bring up, so the last
    // visible line counts instead — otherwise clicking the final section would
    // leave the outline pointing at the head of the document.
    const QScrollBar *bar = verticalScrollBar();
    const bool atBottom = bar->maximum() > 0 && bar->value() >= bar->maximum();
    const int readingLine = atBottom ? viewport()->height() : kHeadingTopGap;

    int found = -1;
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        if (block.blockFormat().intProperty(MarkdownBuilder::kindKey) != int(MarkdownBuilder::BlockKind::Heading))
            continue;

        QTextCursor cursor(block);
        if (cursorRect(cursor).top() > readingLine)
            break; // this one is still below the fold
        found = block.blockNumber();
    }

    if (found == m_visibleHeading)
        return;
    m_visibleHeading = found;
    emit visibleHeadingChanged(found);
}

void MarkdownView::updateContentMargins()
{
    // Cap the text column and centre it: reserve equal margins on both sides.
    int available = width();
    if (const QScrollBar *bar = verticalScrollBar(); bar && bar->isVisible())
        available -= bar->width();
    available -= 2 * frameWidth();

    const int margin = (std::max)(0, (available - m_style.maxContentWidth) / 2);
    setViewportMargins(margin, 0, margin, 0);
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
    // A fresh edit gives up any earlier "render anyway": keeping a huge document
    // live is exactly what the size policy exists to prevent.
    m_previewRequested = false;

    const int interval = effectiveRenderInterval();
    if (interval == 0) {
        rebuild();
    } else if (!m_renderTimer->isActive()) {
        // Coalesce: the first change arms the timer, later ones are absorbed.
        // A degraded document arms it with a longer interval, so a burst of
        // keystrokes still costs one render rather than one per key.
        m_renderTimer->start(interval);
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

int MarkdownView::effectiveRenderInterval() const
{
    // The interval configured by the caller is the floor; a degraded document
    // may ask for a longer one, so typing costs at most one render per interval
    // instead of one per keystroke.
    if (!m_limits.degraded())
        return m_renderTimer->interval();
    return (std::max)(m_renderTimer->interval(), m_limits.renderIntervalMs());
}

void MarkdownView::setDocumentLimits(const DocumentLimits &limits)
{
    const bool wasPaused = previewPaused();

    m_limits = limits;
    m_previewRequested = false;
    m_codeHighlighter->setLimits(limits.codeHighlightMaxBlockChars(), limits.codeHighlightMaxTotalChars());

    // Entering the paused state is cheap, so put the notice up at once.
    // Leaving it must not redraw what is on screen: that could be the very
    // document we are replacing. The caller's setMarkdown() renders the new one.
    if (previewPaused()) {
        if (!wasPaused)
            rebuild();
    } else {
        m_showingPlaceholder = false;
    }
}

bool MarkdownView::previewPaused() const
{
    return !m_limits.previewEnabled() && !m_previewRequested;
}

void MarkdownView::renderAnyway()
{
    m_previewRequested = true;
    m_renderTimer->stop();
    rebuild();
}

void MarkdownView::rebuild()
{
    if (previewPaused()) {
        // The placeholder is cheap, but rebuilding it on every keystroke is
        // still pointless work: once it is up, leave it alone.
        if (!m_showingPlaceholder)
            buildPlaceholder();
        return;
    }
    m_showingPlaceholder = false;

    QElapsedTimer timer;
    timer.start();

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

    m_lastRenderMs = int(timer.elapsed());
    updateVisibleHeading();
    emit rendered();
}

void MarkdownView::buildPlaceholder()
{
    QTextDocument *target = document();
    target->clear();
    m_gifHandler->syncToDocument(); // stops animations from the previous render

    const ThemeDefinition &theme = m_style.theme;

    QTextCharFormat title;
    title.setForeground(theme.markdown.heading);
    title.setFont(m_style.fonts.headingFont(3));
    title.setFontWeight(QFont::Bold);

    QTextCharFormat body;
    body.setForeground(theme.markdown.weakText);
    body.setFont(m_style.fonts.bodyFont());

    // A link, not a button: the renderer already routes anchors to the viewer,
    // so the action needs no extra widget on top of the document.
    QTextCharFormat action;
    action.setForeground(theme.markdown.link);
    action.setFont(m_style.fonts.bodyFont());
    action.setAnchor(true);
    action.setAnchorHref(QStringLiteral("mmd-action:render-preview"));

    QTextBlockFormat centred;
    centred.setAlignment(Qt::AlignHCenter);

    QTextCursor cursor(target);
    cursor.setBlockFormat(centred);
    cursor.insertText(tr("Preview paused"), title);
    cursor.insertBlock(centred);
    cursor.insertText(
        tr("This document is %1. Rendering it on every keystroke would freeze the window.").arg(QLocale().formattedDataSize(m_limits.sizeBytes())),
        body);
    cursor.insertBlock(centred);
    cursor.insertText(tr("Render anyway"), action);

    m_showingPlaceholder = true;
    m_lastRenderMs = 0;
    updateVisibleHeading();
    emit rendered();
}

void MarkdownView::setFindMatches(const QVector<FindState::Match> &matches, int currentIndex)
{
    m_findMatches = matches;
    m_findCurrent = currentIndex;
    applyFindSelections();

    if (currentIndex < 0 || currentIndex >= m_findMatches.size())
        return;

    // A read-only pane keeps no caret of its own, so the hit is brought into
    // view by scrolling: cursorRect() reports where the match ended up.
    const FindState::Match &match = m_findMatches.at(currentIndex);
    QTextCursor cursor(document());
    cursor.setPosition(match.position);
    const int top = cursorRect(cursor).top();
    verticalScrollBar()->setValue(verticalScrollBar()->value() + top - height() / 3);
}

void MarkdownView::clearFindMatches()
{
    if (m_findMatches.isEmpty() && m_findCurrent < 0)
        return;
    m_findMatches.clear();
    m_findCurrent = -1;
    applyFindSelections();
}

void MarkdownView::applyFindSelections()
{
    if (m_findMatches.isEmpty()) {
        setExtraSelections({});
        return;
    }

    QList<QTextEdit::ExtraSelection> selections;
    selections.reserve(m_findMatches.size());
    for (int i = 0; i < m_findMatches.size(); ++i) {
        const FindState::Match &match = m_findMatches.at(i);
        const bool isCurrent = i == m_findCurrent;

        QTextEdit::ExtraSelection selection;
        QColor background = palette().color(QPalette::Highlight);
        if (!isCurrent)
            background.setAlpha(90); // a hint of the same theme colour
        selection.format.setBackground(background);
        if (isCurrent)
            selection.format.setForeground(palette().color(QPalette::HighlightedText));
        selection.cursor = QTextCursor(document());
        selection.cursor.setPosition(match.position);
        selection.cursor.setPosition(match.position + match.length, QTextCursor::KeepAnchor);
        selections.append(selection);
    }
    setExtraSelections(selections);
}
QVector<QTextFrame *> MarkdownView::codeFrames() const
{
    QVector<QTextFrame *> frames;
    std::function<void(QTextFrame *)> walk = [&](QTextFrame *frame) {
        for (QTextFrame *child : frame->childFrames()) {
            if (child->frameFormat().intProperty(MarkdownBuilder::kindKey) == int(MarkdownBuilder::BlockKind::Code))
                frames.append(child);
            walk(child);
        }
    };
    walk(document()->rootFrame());
    return frames;
}

QRect MarkdownView::codeFrameRect(QTextFrame *frame) const
{
    const QRectF docRect = document()->documentLayout()->frameBoundingRect(frame);
    return docRect.translated(-horizontalScrollBar()->value(), -verticalScrollBar()->value()).toRect();
}

QRect MarkdownView::copyButtonRect(QTextFrame *frame) const
{
    constexpr int kSize = 26;
    constexpr int kInset = 6;
    const QRect frameRect = codeFrameRect(frame);
    return QRect(frameRect.right() - kInset - kSize, frameRect.top() + kInset, kSize, kSize);
}

QString MarkdownView::codeText(QTextFrame *frame) const
{
    QString text;
    for (QTextFrame::iterator it = frame->begin(); !it.atEnd(); ++it) {
        const QTextBlock block = it.currentBlock();
        if (!block.isValid())
            continue;
        if (!text.isEmpty())
            text += QLatin1Char('\n');
        text += block.text();
    }
    return text;
}

bool MarkdownView::copyCodeAt(const QPoint &viewportPos)
{
    if (!m_hoveredFrame || !copyButtonRect(m_hoveredFrame).contains(viewportPos))
        return false;

    QGuiApplication::clipboard()->setText(codeText(m_hoveredFrame));
    m_copiedFrame = m_hoveredFrame;
    m_copyFeedbackTimer->start();
    viewport()->update();
    return true;
}

void MarkdownView::updateHoveredFrame(const QPoint &pos)
{
    QTextFrame *hovered = nullptr;
    for (QTextFrame *frame : codeFrames()) {
        if (codeFrameRect(frame).contains(pos)) {
            hovered = frame;
            break;
        }
    }

    if (hovered != m_hoveredFrame) {
        m_hoveredFrame = hovered;
        viewport()->update();
    }

    const bool overButton = m_hoveredFrame && copyButtonRect(m_hoveredFrame).contains(pos);
    viewport()->setCursor(overButton ? Qt::PointingHandCursor : Qt::IBeamCursor);
}

void MarkdownView::mouseMoveEvent(QMouseEvent *event)
{
    updateHoveredFrame(event->pos());
    QTextBrowser::mouseMoveEvent(event);
}

void MarkdownView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && copyCodeAt(event->pos()))
        return;
    QTextBrowser::mousePressEvent(event);
}

void MarkdownView::leaveEvent(QEvent *event)
{
    if (m_hoveredFrame) {
        m_hoveredFrame = nullptr;
        viewport()->update();
    }
    QTextBrowser::leaveEvent(event);
}

void MarkdownView::paintEvent(QPaintEvent *event)
{
    QTextBrowser::paintEvent(event);

    if (!m_hoveredFrame)
        return;

    QPainter painter(viewport());
    drawCopyButton(painter, copyButtonRect(m_hoveredFrame), m_hoveredFrame == m_copiedFrame);
}

void MarkdownView::drawCopyButton(QPainter &painter, const QRect &button, bool copied) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, false); // sharp, flat corners

    const QColor ink = m_style.theme.markdown.codeText;
    constexpr float kChipAlpha = 30.0f / 255.0f;
    QColor chip = ink;
    chip.setAlpha(30);
    QColor border = ink;
    border.setAlpha(56);
    painter.setPen(QPen(border, 1));
    painter.setBrush(chip);
    painter.drawRect(button.adjusted(0, 0, -1, -1)); // sharp square, no rounding

    const QColor glyph = m_style.theme.markdown.weakText;
    if (copied) {
        // A sharp tick.
        painter.setPen(QPen(glyph, 2));
        painter.drawLine(button.left() + 8, button.top() + 13, button.left() + 11, button.top() + 16);
        painter.drawLine(button.left() + 11, button.top() + 16, button.left() + 18, button.top() + 9);
    } else {
        // Two overlapping sheets; the front one is filled to occlude the back.
        const QColor under = m_style.theme.markdown.codeBackground;
        const QColor chipOpaque = QColor::fromRgbF(under.redF() * (1 - kChipAlpha) + ink.redF() * kChipAlpha,
                                                   under.greenF() * (1 - kChipAlpha) + ink.greenF() * kChipAlpha,
                                                   under.blueF() * (1 - kChipAlpha) + ink.blueF() * kChipAlpha);
        const QRect back(button.left() + 7, button.top() + 6, 9, 10);
        const QRect front(button.left() + 11, button.top() + 10, 9, 10);
        painter.setPen(QPen(glyph, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(back);
        painter.setBrush(chipOpaque);
        painter.setPen(Qt::NoPen);
        painter.drawRect(front);
        painter.setPen(QPen(glyph, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(front);
    }

    painter.restore();
}

bool MarkdownView::scrollToBlock(int blockNumber)
{
    const QTextBlock block = document()->findBlockByNumber(blockNumber);
    if (!block.isValid())
        return false;

    QTextCursor cursor(block);
    const int top = cursorRect(cursor).top();
    // Land the heading at the top edge, with the same gap the "current section"
    // rule tolerates, so the outline marks the section it just jumped to.
    verticalScrollBar()->setValue(verticalScrollBar()->value() + top - kHeadingTopGap);
    return true;
}

bool MarkdownView::scrollToAnchor(const QString &slug)
{
    for (QTextBlock block = document()->firstBlock(); block.isValid(); block = block.next()) {
        if (block.blockFormat().intProperty(MarkdownBuilder::kindKey) != int(MarkdownBuilder::BlockKind::Heading))
            continue;
        if (MarkdownBuilder::headingSlug(block.text()) != slug)
            continue;

        QTextCursor cursor(block);
        const int top = cursorRect(cursor).top();
        verticalScrollBar()->setValue(verticalScrollBar()->value() + top - 8);
        return true;
    }
    return false;
}

void MarkdownView::handleAnchor(const QUrl &url)
{
    // Our own links (the paused-preview action) never leave the application.
    if (url.scheme() == QLatin1String("mmd-action")) {
        if (url.path() == QLatin1String("render-preview"))
            renderAnyway();
        return;
    }

    // In-document anchors scroll the preview instead of navigating away (which
    // would blank the view).
    if (url.hasFragment() && (url.path().isEmpty() || url.scheme() == QLatin1String("file"))) {
        if (scrollToAnchor(url.fragment()))
            return;
    }

    if (url.scheme() == QLatin1String("http") || url.scheme() == QLatin1String("https"))
        QDesktopServices::openUrl(url);
    // Other schemes (mailto, relative files) are left untouched on purpose.
}
