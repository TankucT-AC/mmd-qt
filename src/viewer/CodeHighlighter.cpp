// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/CodeHighlighter.h"

#include "core/MarkdownBuilder.h"

#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/State>
#include <KSyntaxHighlighting/Theme>

#include <QTextCursor>
#include <QTextDocument>

using namespace KSyntaxHighlighting;

// Common fence labels that do not map to a file extension the way KF6
// expects. Anything else is tried verbatim.
namespace
{

QString canonicalLanguage(const QString &language)
{
    static const QHash<QString, QString> aliases = {
        {QStringLiteral("c++"), QStringLiteral("cpp")},
        {QStringLiteral("cxx"), QStringLiteral("cpp")},
        {QStringLiteral("c"), QStringLiteral("cpp")},
        {QStringLiteral("py"), QStringLiteral("python")},
        {QStringLiteral("js"), QStringLiteral("javascript")},
        {QStringLiteral("ts"), QStringLiteral("typescript")},
        {QStringLiteral("sh"), QStringLiteral("bash")},
        {QStringLiteral("shell"), QStringLiteral("bash")},
        {QStringLiteral("yml"), QStringLiteral("yaml")},
        {QStringLiteral("md"), QStringLiteral("markdown")},
        {QStringLiteral("qml"), QStringLiteral("qml")},
    };
    return aliases.value(language.toLower(), language);
}

} // namespace

void CodeHighlighter::setThemeDefinition(const ThemeDefinition &theme)
{
    m_themeDefinition = theme;

    Theme syntaxTheme = m_repository.theme(theme.syntaxThemeName);
    if (!syntaxTheme.isValid())
        syntaxTheme = m_repository.defaultTheme(Repository::LightTheme);
    setTheme(syntaxTheme);
}

Definition CodeHighlighter::definitionFor(const QString &language) const
{
    if (language.isEmpty())
        return Definition();

    const QString canonical = canonicalLanguage(language);
    Definition definition = m_repository.definitionForFileName(QStringLiteral("snippet.") + canonical);
    if (!definition.isValid())
        definition = m_repository.definitionForName(canonical);
    if (!definition.isValid())
        definition = m_repository.definitionForName(language);
    return definition;
}

void CodeHighlighter::highlight(QTextDocument *document)
{
    Definition currentDefinition;
    State state;

    for (QTextBlock block = document->firstBlock(); block.isValid(); block = block.next()) {
        const QTextBlockFormat format = block.blockFormat();
        if (format.intProperty(MarkdownBuilder::kindKey) != int(MarkdownBuilder::BlockKind::Code)) {
            currentDefinition = Definition();
            continue;
        }

        const bool startsRegion = block.previous().blockFormat().intProperty(MarkdownBuilder::kindKey) != int(MarkdownBuilder::BlockKind::Code);
        const QString language = format.stringProperty(MarkdownBuilder::languageKey);

        if (startsRegion) {
            currentDefinition = definitionFor(language);
            state = State();
            setDefinition(currentDefinition);
        }

        if (!currentDefinition.isValid())
            continue;

        m_block = block;
        state = highlightLine(block.text(), state);
    }
}

void CodeHighlighter::applyFormat(int offset, int length, const Format &format)
{
    if (length <= 0)
        return;

    QTextCharFormat charFormat;
    if (format.hasTextColor(theme()))
        charFormat.setForeground(format.textColor(theme()));
    if (format.isBold(theme()))
        charFormat.setFontWeight(QFont::Bold);
    if (format.isItalic(theme()))
        charFormat.setFontItalic(true);
    if (charFormat.isEmpty())
        return;

    QTextCursor cursor(m_block);
    const int start = m_block.position() + offset;
    cursor.setPosition(start);
    cursor.setPosition(start + length, QTextCursor::KeepAnchor);
    cursor.mergeCharFormat(charFormat);
}