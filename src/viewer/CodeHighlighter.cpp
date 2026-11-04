// SPDX-FileCopyrightText: 2026 Total Markdown contributors
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
    // A default-constructed theme has no code colors yet; fall back to light so
    // highlighting still works before a real theme is applied.
    if (!theme.syntax.keyword.isValid())
        m_themeDefinition.syntax = ThemeDefinition::light().syntax;

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

void CodeHighlighter::setLimits(int maxBlockChars, int maxTotalChars)
{
    m_maxBlockChars = maxBlockChars;
    m_maxTotalChars = maxTotalChars;
}

void CodeHighlighter::highlight(QTextDocument *document)
{
    Definition currentDefinition;
    State state;
    int highlightedChars = 0;

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

        if (m_maxTotalChars >= 0 && highlightedChars >= m_maxTotalChars)
            break;

        const QString text = block.text();
        const qsizetype length = text.size();
        if (m_maxBlockChars >= 0 && length > qsizetype(m_maxBlockChars)) {
            // One enormous line is where the cost explodes. Drop the rest of the
            // region rather than pay for it: a partly highlighted block would be
            // more confusing than a plain one.
            currentDefinition = Definition();
            continue;
        }

        highlightedChars += int(length); // bounded by m_maxBlockChars when it is set
        m_block = block;
        state = highlightLine(text, state);
    }
}

QColor CodeHighlighter::syntaxColor(Theme::TextStyle style) const
{
    const SyntaxColors &s = m_themeDefinition.syntax;
    switch (style) {
    case Theme::Keyword:
    case Theme::ControlFlow:
    case Theme::Import:
        return s.keyword;
    case Theme::Function:
        return s.function;
    case Theme::Variable:
        return s.variable;
    case Theme::BuiltIn:
    case Theme::Extension:
    case Theme::DataType:
        return s.type;
    case Theme::DecVal:
    case Theme::BaseN:
    case Theme::Float:
    case Theme::Constant:
        return s.constant;
    case Theme::Comment:
    case Theme::Documentation:
    case Theme::CommentVar:
    case Theme::RegionMarker:
    case Theme::Information:
        return s.comment;
    case Theme::Char:
    case Theme::SpecialChar:
    case Theme::String:
    case Theme::VerbatimString:
    case Theme::SpecialString:
        return s.string;
    case Theme::Preprocessor:
        return s.preprocessor;
    case Theme::Operator:
        return s.op;
    case Theme::Attribute:
    case Theme::Annotation:
        return s.annotation;
    case Theme::Warning:
    case Theme::Alert:
        return s.warning;
    case Theme::Error:
        return s.error;
    default:
        return QColor(); // Normal and anything unknown keeps the block's color
    }
}

void CodeHighlighter::applyFormat(int offset, int length, const Format &format)
{
    if (length <= 0)
        return;

    QTextCharFormat charFormat;
    const QColor color = syntaxColor(format.textStyle());
    if (color.isValid())
        charFormat.setForeground(color);
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