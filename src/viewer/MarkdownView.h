// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/MarkdownBuilder.h"

#include <QTextBrowser>

#include <memory>

class QTimer;
class GifObjectHandler;
class CodeHighlighter;
class CheckboxObjectHandler;
class ChipObjectHandler;

/*
 * Preview pane: renders markdown source into its QTextDocument and keeps the
 * result up to date as the source changes.
 *
 * Rendering is incremental in the sense that matters for a live preview:
 * repeated setMarkdown() calls within the debounce window collapse into a
 * single rebuild, so a burst of keystrokes costs one render, not one per key.
 * The scroll position is preserved across a rebuild by anchoring on the block
 * currently at the top of the viewport.
 *
 * Rebuilding the document in place (rather than swapping in a new
 * QTextDocument) keeps the QTextObjectInterface handler registration valid.
 */
class MarkdownView : public QTextBrowser
{
    Q_OBJECT

public:
    explicit MarkdownView(QWidget *parent = nullptr);
    ~MarkdownView() override;

    void setRenderStyle(const RenderStyle &style);
    const RenderStyle &renderStyle() const;

    void setBaseDir(const QString &directory);

    // Schedules a render; calls within the interval are coalesced.
    void setMarkdown(const QString &markdown);

    // Renders immediately, bypassing the debounce.
    void renderNow();

    // Re-renders the current source through the debounce, so a burst of
    // layout changes (e.g. dragging the splitter) coalesces into one render.
    void scheduleRerender();

    int renderInterval() const;
    void setRenderInterval(int milliseconds);

    // Code boxes of the current document, in document order. Each is the
    // QTextFrame the builder wrapped a fenced code block in.
    QVector<QTextFrame *> codeFrames() const;

    // The screen (viewport) rectangle of a code box and of its copy button.
    QRect codeFrameRect(QTextFrame *frame) const;
    QRect copyButtonRect(QTextFrame *frame) const;

    // Plain text of a code box.
    QString codeText(QTextFrame *frame) const;

    // Copies the code box under the given viewport point, if the point hits its
    // copy button. Returns true when something was copied.
    bool copyCodeAt(const QPoint &viewportPos);

    // Scrolls to the heading whose GitHub anchor is `slug`. Returns false when
    // no heading matches.
    bool scrollToAnchor(const QString &slug);

signals:
    void rendered();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void rebuild();
    void updateContentMargins();
    void updateHoveredFrame(const QPoint &pos);
    void drawCopyButton(QPainter &painter, const QRect &button, bool copied) const;
    void handleAnchor(const QUrl &url);

    RenderStyle m_style;
    QString m_source;
    QTimer *m_renderTimer = nullptr;
    GifObjectHandler *m_gifHandler = nullptr;
    CheckboxObjectHandler *m_checkboxHandler = nullptr;
    ChipObjectHandler *m_chipHandler = nullptr;
    std::unique_ptr<CodeHighlighter> m_codeHighlighter;

    QTextFrame *m_hoveredFrame = nullptr;
    QTextFrame *m_copiedFrame = nullptr;
    QTimer *m_copyFeedbackTimer = nullptr;
};