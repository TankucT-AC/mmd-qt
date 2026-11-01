// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/DocumentLimits.h"
#include "core/FindState.h"
#include "core/MarkdownBuilder.h"

#include <QTextBrowser>

#include <memory>

class QTimer;
class GifObjectHandler;
class CodeHighlighter;
class CheckboxObjectHandler;
class ChipObjectHandler;
class MathObjectHandler;

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

    // Applies the document-size policy. For a document past the preview limit
    // the pane shows a short explanation instead of the render, so editing
    // stays responsive; renderAnyway() overrides that once.
    void setDocumentLimits(const DocumentLimits &limits);

    // Renders the current source even though the limits said not to. Wired to
    // the "Render anyway" link of the paused-preview placeholder.
    void renderAnyway();

    // True while the source is too big to render without an explicit request.
    bool previewPaused() const;

    // Scrolls the heading that lives in this rendered block to the top of the
    // pane. Returns false when no such block exists.
    bool scrollToBlock(int blockNumber);

    // The heading currently at the top of the pane, as a block number, or -1
    // while above the first heading.
    int visibleHeading() const
    {
        return m_visibleHeading;
    }

    // Duration of the last rebuild in milliseconds; 0 while the preview is
    // paused. Surfaced in the status bar.
    int lastRenderMs() const
    {
        return m_lastRenderMs;
    }

    // Search decorations. The pane is read-only, so it highlights the hits and
    // scrolls the current one into view instead of moving a caret.
    void setFindMatches(const QVector<FindState::Match> &matches, int currentIndex);
    void clearFindMatches();

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

    // The heading the reader is currently under, as a rendered block number, or
    // -1 while above the first heading. Drives the outline panel.
    void visibleHeadingChanged(int blockNumber);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void rebuild();
    void buildPlaceholder();
    void applyFindSelections();
    void updateVisibleHeading();
    int effectiveRenderInterval() const;
    void updateContentMargins();
    void updateHoveredFrame(const QPoint &pos);
    void drawCopyButton(QPainter &painter, const QRect &button, bool copied) const;
    void handleAnchor(const QUrl &url);

    RenderStyle m_style;
    DocumentLimits m_limits;
    QString m_source;
    QTimer *m_renderTimer = nullptr;
    GifObjectHandler *m_gifHandler = nullptr;
    CheckboxObjectHandler *m_checkboxHandler = nullptr;
    ChipObjectHandler *m_chipHandler = nullptr;
    MathObjectHandler *m_mathHandler = nullptr;
    std::unique_ptr<CodeHighlighter> m_codeHighlighter;

    QTextFrame *m_hoveredFrame = nullptr;
    QTextFrame *m_copiedFrame = nullptr;
    QTimer *m_copyFeedbackTimer = nullptr;
    bool m_showingPlaceholder = false;
    bool m_previewRequested = false; // the user asked for this one render
    int m_lastRenderMs = 0;
    QVector<FindState::Match> m_findMatches;
    int m_findCurrent = -1;
    int m_visibleHeading = -1;
};