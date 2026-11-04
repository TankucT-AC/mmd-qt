// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "editor/MarkdownHighlighter.h"

#include <QTextDocument>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document)
    , m_fenceLine(QRegularExpression(QStringLiteral("^\\s*(`{3,}|~{3,})")))
    , m_heading(QRegularExpression(QStringLiteral("^\\s{0,3}(#{1,6})\\s+.*$")))
    , m_bold(QRegularExpression(QStringLiteral("(\\*\\*[^*\\n]+\\*\\*|__[^_\\n]+__)")))
    , m_emphasis(QRegularExpression(QStringLiteral("(\\*[^*\\n]+\\*|_[^_\\n]+_)")))
    , m_inlineCode(QRegularExpression(QStringLiteral("`[^`\\n]+`")))
    , m_image(QRegularExpression(QStringLiteral("!\\[[^\\]\\n]*\\]\\([^)\\n]+\\)")))
    , m_link(QRegularExpression(QStringLiteral("\\[[^\\]\\n]*\\]\\([^)\\n]+\\)")))
    , m_quoteMarker(QRegularExpression(QStringLiteral("^(\\s{0,3}>)+")))
    , m_taskMarker(QRegularExpression(QStringLiteral("^(\\s{0,3}([-*+]|\\d{1,9}[.)])\\s+)(\\[[ xX]\\])(\\s|$)")))
    , m_listMarker(QRegularExpression(QStringLiteral("^\\s{0,3}([-*+]|\\d{1,9}[.)])(\\s+|$)")))
    , m_horizontalRule(QRegularExpression(QStringLiteral("^\\s{0,3}((-\\s*){3,}|(\\*\\s*){3,}|(_\\s*){3,})$")))
{
}

void MarkdownHighlighter::setTheme(const ThemeDefinition &theme)
{
    m_theme = theme;
    rehighlight();
}

QTextCharFormat MarkdownHighlighter::makeFormat(QColor color, QFont::Weight weight) const
{
    QTextCharFormat format;
    format.setForeground(color);
    format.setFontWeight(weight);
    return format;
}

void MarkdownHighlighter::setFenceLineFormat(const QString &text)
{
    QTextCharFormat format;
    format.setForeground(m_theme.markdown.codeText);
    format.setBackground(m_theme.markdown.codeBackground);
    setFormat(0, int(text.size()), format);
}

void MarkdownHighlighter::applyLineRules(const QString &text)
{
    if (m_fenceLine.match(text).hasMatch()) {
        setFenceLineFormat(text);
        return; // fence markers do not get other styling
    }
    if (m_heading.match(text).hasMatch()) {
        setFormat(0, int(text.size()), makeFormat(m_theme.markdown.heading, QFont::Bold));
        return;
    }
    if (m_horizontalRule.match(text).hasMatch()) {
        setFormat(0, int(text.size()), makeFormat(m_theme.markdown.rule));
        return;
    }

    const QRegularExpressionMatch quote = m_quoteMarker.match(text);
    if (quote.hasMatch())
        setFormat(0, int(quote.capturedLength()), makeFormat(m_theme.markdown.quoteBar, QFont::Bold));

    const QRegularExpressionMatch task = m_taskMarker.match(text);
    if (task.hasMatch()) {
        const QString mark = task.captured(3);
        const QColor color = mark.compare(QStringLiteral("[x]"), Qt::CaseInsensitive) == 0 ? m_theme.markdown.taskDone : m_theme.markdown.taskOpen;
        setFormat(int(task.capturedStart(3)), int(task.capturedLength(3)), makeFormat(color, QFont::Bold));
    } else {
        const QRegularExpressionMatch list = m_listMarker.match(text);
        if (list.hasMatch())
            setFormat(0, int(list.capturedLength()), makeFormat(m_theme.markdown.weakText, QFont::Bold));
    }
}

void MarkdownHighlighter::applySpanRules(const QString &text)
{
    QTextCharFormat codeFormat;
    codeFormat.setForeground(m_theme.markdown.codeText);
    codeFormat.setBackground(m_theme.markdown.codeBackground);

    QRegularExpressionMatchIterator it = m_inlineCode.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        setFormat(int(match.capturedStart()), int(match.capturedLength()), codeFormat);
    }

    it = m_bold.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        QTextCharFormat format = makeFormat(m_theme.palette.color(QPalette::Text), QFont::Bold);
        setFormat(int(match.capturedStart()), int(match.capturedLength()), format);
    }

    it = m_emphasis.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        QTextCharFormat format;
        format.setFontItalic(true);
        setFormat(int(match.capturedStart()), int(match.capturedLength()), format);
    }

    it = m_image.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        setFormat(int(match.capturedStart()), int(match.capturedLength()), makeFormat(m_theme.markdown.taskOpen));
    }

    it = m_link.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        setFormat(int(match.capturedStart()), int(match.capturedLength()), makeFormat(m_theme.markdown.link));
    }
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    const int previousState = previousBlockState();

    if (previousState == StateInFence) {
        setFenceLineFormat(text);
        setCurrentBlockState(m_fenceLine.match(text).hasMatch() ? StateNormal : StateInFence);
        return;
    }

    setCurrentBlockState(StateNormal);
    if (m_fenceLine.match(text).hasMatch()) {
        setFenceLineFormat(text);
        setCurrentBlockState(StateInFence);
        return;
    }

    applyLineRules(text);
    applySpanRules(text);
}
