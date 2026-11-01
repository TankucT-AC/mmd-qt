// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/DocumentLimits.h"
#include "core/FindState.h"
#include "core/Theme.h"

#include <QPlainTextEdit>

class MarkdownHighlighter;

/*
 * Source pane of the two-pane view: a plain text editor with line numbers,
 * current-line highlighting and markdown syntax highlighting. Editing is the
 * input side of the live preview; the editor itself knows nothing about
 * rendering markdown.
 */
class MarkdownEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit MarkdownEditor(QWidget *parent = nullptr);

    void setTheme(const ThemeDefinition &theme);

    // Applies the document-size policy. Above a threshold the markdown
    // highlighter is detached: re-highlighting a huge file on every keystroke
    // costs far more than the colours are worth.
    void setDocumentLimits(const DocumentLimits &limits);

    void setHighlightingEnabled(bool enabled);
    bool highlightingEnabled() const
    {
        return m_highlightingEnabled;
    }

    // Search decorations. The editor owns them because Qt allows only a single
    // ExtraSelections list: the current-line tint and the search hits have to be
    // composed into one call. The caret follows the current hit, so the user can
    // keep typing from there.
    void setFindMatches(const QVector<FindState::Match> &matches, int currentIndex);
    void clearFindMatches();

    // Replaces the whole document and re-highlights. Programmatic text
    // replacement needs an explicit rehighlight: the synchronous pass Qt runs
    // during setPlainText writes to a layout that is discarded afterwards.
    void setMarkdown(const QString &markdown);

    int lineNumberAreaWidth() const;

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth();
    void updateLineNumberArea(const QRect &rect, int dy);
    void applyExtraSelections();

private:
    class LineNumberArea;

    void lineNumberAreaPaintEvent(QPaintEvent *event);

    MarkdownHighlighter *m_highlighter = nullptr;
    LineNumberArea *m_lineNumberArea = nullptr;
    ThemeDefinition m_theme;
    bool m_highlightingEnabled = true;
    QVector<FindState::Match> m_findMatches;
    int m_findCurrent = -1;
};
