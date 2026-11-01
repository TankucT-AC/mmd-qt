// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/MarkdownView.h"

#include "core/FontManager.h"
#include "viewer/CheckboxObjectHandler.h"
#include "viewer/ChipObjectHandler.h"
#include "viewer/CodeHighlighter.h"
#include "viewer/GifObjectHandler.h"

#include <QAbstractTextDocumentLayout>
#include <QClipboard>
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextFrame>
#include <QTimer>

#include <algorithm>
#include <functional>

namespace
{
// Coalesces bursts of edits into a single render; small enough to feel instant.
constexpr int kDefaultRenderIntervalMs = 40;
}

MarkdownView::MarkdownView(QWidget *parent)
    : QTextBrowser(parent)
    , m_gifHandler(new GifObjectHandler(document(), this))
    , m_checkboxHandler(new CheckboxObjectHandler(this))
    , m_chipHandler(new ChipObjectHandler(this))
    , m_codeHighlighter(new CodeHighlighter)
{
    setOpenExternalLinks(true);
    setOpenLinks(true);
    setFrameShape(QFrame::NoFrame);

    document()->documentLayout()->registerHandler(MarkdownBuilder::gifObjectType, m_gifHandler);
    document()->documentLayout()->registerHandler(MarkdownBuilder::checkboxObjectType, m_checkboxHandler);
    document()->documentLayout()->registerHandler(MarkdownBuilder::chipObjectType, m_chipHandler);

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
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor ink = m_style.theme.markdown.codeText;
    constexpr qreal kChipAlpha = 26.0 / 255.0;
    QColor chip = ink;
    chip.setAlpha(26);
    QColor border = ink;
    border.setAlpha(48);
    painter.setPen(QPen(border, 1));
    painter.setBrush(chip);
    painter.drawRoundedRect(button.adjusted(0, 0, -1, -1), 6, 6);

    const QColor glyph = m_style.theme.markdown.weakText;
    if (copied) {
        QPainterPath tick;
        tick.moveTo(button.left() + 8.0, button.top() + 13.5);
        tick.lineTo(button.left() + 11.5, button.top() + 17.0);
        tick.lineTo(button.left() + 18.0, button.top() + 9.5);
        painter.setPen(QPen(glyph, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(tick);
    } else {
        // Two overlapping sheets: the front one is filled to occlude the back.
        const QColor under = m_style.theme.markdown.codeBackground;
        const QColor chipOpaque = QColor::fromRgbF(under.redF() * (1 - kChipAlpha) + ink.redF() * kChipAlpha,
                                                   under.greenF() * (1 - kChipAlpha) + ink.greenF() * kChipAlpha,
                                                   under.blueF() * (1 - kChipAlpha) + ink.blueF() * kChipAlpha);
        const QRectF back(button.left() + 7.0, button.top() + 6.0, 9.0, 11.0);
        const QRectF front(button.left() + 11.0, button.top() + 10.0, 9.0, 11.0);
        painter.setPen(QPen(glyph, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(back, 2.5, 2.5);
        painter.setBrush(chipOpaque);
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(front, 2.5, 2.5);
        painter.setPen(QPen(glyph, 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(front, 2.5, 2.5);
    }

    painter.restore();
}
