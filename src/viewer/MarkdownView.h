// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/MarkdownBuilder.h"

#include <QTextBrowser>

class QTimer;
class GifObjectHandler;

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

    void setRenderStyle(const RenderStyle &style);
    const RenderStyle &renderStyle() const;

    void setBaseDir(const QString &directory);

    // Schedules a render; calls within the interval are coalesced.
    void setMarkdown(const QString &markdown);

    // Renders immediately, bypassing the debounce.
    void renderNow();

    int renderInterval() const;
    void setRenderInterval(int milliseconds);

signals:
    void rendered();

private:
    void rebuild();

    RenderStyle m_style;
    QString m_source;
    QTimer *m_renderTimer = nullptr;
    GifObjectHandler *m_gifHandler = nullptr;
};