// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/Theme.h"

#include <QJsonArray>
#include <QVector>

namespace
{

QPalette makePalette(const QColor &window,
                     const QColor &windowText,
                     const QColor &base,
                     const QColor &text,
                     const QColor &button,
                     const QColor &buttonText,
                     const QColor &highlight,
                     const QColor &highlightedText,
                     const QColor &link,
                     const QColor &linkVisited,
                     const QColor &placeholder)
{
    QPalette palette;
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, windowText);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, window);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, button);
    palette.setColor(QPalette::ButtonText, buttonText);
    palette.setColor(QPalette::ToolTipBase, text);
    palette.setColor(QPalette::ToolTipText, base);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, highlightedText);
    palette.setColor(QPalette::Link, link);
    palette.setColor(QPalette::LinkVisited, linkVisited);
    palette.setColor(QPalette::PlaceholderText, placeholder);
    return palette;
}

QColor color(const QJsonObject &object, const QString &key)
{
    const QString value = object.value(key).toString();
    return value.isEmpty() ? QColor() : QColor(value);
}

constexpr QPalette::ColorGroup active = QPalette::Active;

} // namespace

ThemeDefinition ThemeDefinition::light()
{
    ThemeDefinition theme;
    theme.name = QStringLiteral("light");
    theme.syntaxThemeName = QStringLiteral("Breeze Light");

    const QColor window(0xfc, 0xfc, 0xfc);
    const QColor windowText(0x1f, 0x23, 0x28);
    const QColor base(0xff, 0xff, 0xff);
    const QColor weak(0x6e, 0x77, 0x81);
    const QColor link(0x09, 0x69, 0xda);
    theme.palette =
        makePalette(window, windowText, base, windowText, QColor(0xf0, 0xf0, 0xf0), windowText, link, Qt::white, link, QColor(0x82, 0x50, 0xdf), weak);

    theme.markdown.heading = windowText;
    theme.markdown.weakText = weak;
    theme.markdown.link = link;
    theme.markdown.codeBackground = QColor(0xf3, 0xf4, 0xf6);
    theme.markdown.codeText = windowText;
    theme.markdown.quoteBackground = QColor(0xf6, 0xf8, 0xfa);
    theme.markdown.quoteBar = QColor(0xd0, 0xd7, 0xde);
    theme.markdown.rule = QColor(0xd0, 0xd7, 0xde);
    theme.markdown.tableBorder = QColor(0xd0, 0xd7, 0xde);
    theme.markdown.tableHeaderBackground = QColor(0xf3, 0xf4, 0xf6);
    theme.markdown.taskOpen = weak;
    theme.markdown.taskDone = QColor(0x1a, 0x7f, 0x37);

    // GitHub-like syntax palette (light).
    theme.syntax.keyword = QColor(0xcf, 0x22, 0x2e);
    theme.syntax.function = QColor(0x82, 0x50, 0xdf);
    theme.syntax.variable = QColor(0x95, 0x38, 0x00);
    theme.syntax.type = QColor(0x95, 0x38, 0x00);
    theme.syntax.constant = QColor(0x05, 0x50, 0xae);
    theme.syntax.comment = QColor(0x6e, 0x77, 0x81);
    theme.syntax.string = QColor(0x0a, 0x30, 0x69);
    theme.syntax.preprocessor = QColor(0x05, 0x50, 0xae);
    theme.syntax.op = QColor(0x05, 0x50, 0xae);
    theme.syntax.annotation = QColor(0x82, 0x50, 0xdf);
    theme.syntax.warning = QColor(0x9a, 0x67, 0x00);
    theme.syntax.error = QColor(0xcf, 0x22, 0x2e);
    return theme;
}

ThemeDefinition ThemeDefinition::dark()
{
    ThemeDefinition theme;
    theme.name = QStringLiteral("dark");
    theme.syntaxThemeName = QStringLiteral("Breeze Dark");

    const QColor window(0x16, 0x19, 0x1f);
    const QColor windowText(0xe6, 0xed, 0xf3);
    const QColor base(0x1c, 0x20, 0x28);
    const QColor weak(0x8b, 0x94, 0x9e);
    const QColor link(0x58, 0xa6, 0xff);
    theme.palette =
        makePalette(window, windowText, base, windowText, QColor(0x26, 0x2b, 0x35), windowText, link, Qt::white, link, QColor(0xd2, 0xa8, 0xff), weak);

    theme.markdown.heading = windowText;
    theme.markdown.weakText = weak;
    theme.markdown.link = link;
    theme.markdown.codeBackground = QColor(0x22, 0x27, 0x2e);
    theme.markdown.codeText = windowText;
    theme.markdown.quoteBackground = QColor(0x1c, 0x21, 0x28);
    theme.markdown.quoteBar = QColor(0x3d, 0x44, 0x4d);
    theme.markdown.rule = QColor(0x3d, 0x44, 0x4d);
    theme.markdown.tableBorder = QColor(0x3d, 0x44, 0x4d);
    theme.markdown.tableHeaderBackground = QColor(0x22, 0x27, 0x2e);
    theme.markdown.taskOpen = weak;
    theme.markdown.taskDone = QColor(0x3f, 0xb9, 0x50);

    // GitHub-like syntax palette (dark).
    theme.syntax.keyword = QColor(0xff, 0x7b, 0x72);
    theme.syntax.function = QColor(0xd2, 0xa8, 0xff);
    theme.syntax.variable = QColor(0xff, 0xa6, 0x57);
    theme.syntax.type = QColor(0xff, 0xa6, 0x57);
    theme.syntax.constant = QColor(0x79, 0xc0, 0xff);
    theme.syntax.comment = QColor(0x8b, 0x94, 0x9e);
    theme.syntax.string = QColor(0xa5, 0xd6, 0xff);
    theme.syntax.preprocessor = QColor(0x79, 0xc0, 0xff);
    theme.syntax.op = QColor(0x79, 0xc0, 0xff);
    theme.syntax.annotation = QColor(0xd2, 0xa8, 0xff);
    theme.syntax.warning = QColor(0xd2, 0x99, 0x22);
    theme.syntax.error = QColor(0xff, 0x7b, 0x72);
    return theme;
}

QJsonObject ThemeDefinition::toJson() const
{
    QJsonObject object;
    object.insert(QStringLiteral("name"), name);
    object.insert(QStringLiteral("syntaxThemeName"), syntaxThemeName);

    QJsonObject paletteObject;
    paletteObject.insert(QStringLiteral("window"), palette.color(active, QPalette::Window).name());
    paletteObject.insert(QStringLiteral("windowText"), palette.color(active, QPalette::WindowText).name());
    paletteObject.insert(QStringLiteral("base"), palette.color(active, QPalette::Base).name());
    paletteObject.insert(QStringLiteral("alternateBase"), palette.color(active, QPalette::AlternateBase).name());
    paletteObject.insert(QStringLiteral("text"), palette.color(active, QPalette::Text).name());
    paletteObject.insert(QStringLiteral("button"), palette.color(active, QPalette::Button).name());
    paletteObject.insert(QStringLiteral("buttonText"), palette.color(active, QPalette::ButtonText).name());
    paletteObject.insert(QStringLiteral("highlight"), palette.color(active, QPalette::Highlight).name());
    paletteObject.insert(QStringLiteral("highlightedText"), palette.color(active, QPalette::HighlightedText).name());
    paletteObject.insert(QStringLiteral("link"), palette.color(active, QPalette::Link).name());
    paletteObject.insert(QStringLiteral("linkVisited"), palette.color(active, QPalette::LinkVisited).name());
    paletteObject.insert(QStringLiteral("placeholderText"), palette.color(active, QPalette::PlaceholderText).name());
    object.insert(QStringLiteral("palette"), paletteObject);

    QJsonObject markdownObject;
    markdownObject.insert(QStringLiteral("heading"), markdown.heading.name());
    markdownObject.insert(QStringLiteral("weakText"), markdown.weakText.name());
    markdownObject.insert(QStringLiteral("link"), markdown.link.name());
    markdownObject.insert(QStringLiteral("codeBackground"), markdown.codeBackground.name());
    markdownObject.insert(QStringLiteral("codeText"), markdown.codeText.name());
    markdownObject.insert(QStringLiteral("quoteBackground"), markdown.quoteBackground.name());
    markdownObject.insert(QStringLiteral("quoteBar"), markdown.quoteBar.name());
    markdownObject.insert(QStringLiteral("rule"), markdown.rule.name());
    markdownObject.insert(QStringLiteral("tableBorder"), markdown.tableBorder.name());
    markdownObject.insert(QStringLiteral("tableHeaderBackground"), markdown.tableHeaderBackground.name());
    markdownObject.insert(QStringLiteral("taskOpen"), markdown.taskOpen.name());
    markdownObject.insert(QStringLiteral("taskDone"), markdown.taskDone.name());
    object.insert(QStringLiteral("markdown"), markdownObject);

    QJsonObject syntaxObject;
    syntaxObject.insert(QStringLiteral("keyword"), syntax.keyword.name());
    syntaxObject.insert(QStringLiteral("function"), syntax.function.name());
    syntaxObject.insert(QStringLiteral("variable"), syntax.variable.name());
    syntaxObject.insert(QStringLiteral("type"), syntax.type.name());
    syntaxObject.insert(QStringLiteral("constant"), syntax.constant.name());
    syntaxObject.insert(QStringLiteral("comment"), syntax.comment.name());
    syntaxObject.insert(QStringLiteral("string"), syntax.string.name());
    syntaxObject.insert(QStringLiteral("preprocessor"), syntax.preprocessor.name());
    syntaxObject.insert(QStringLiteral("op"), syntax.op.name());
    syntaxObject.insert(QStringLiteral("annotation"), syntax.annotation.name());
    syntaxObject.insert(QStringLiteral("warning"), syntax.warning.name());
    syntaxObject.insert(QStringLiteral("error"), syntax.error.name());
    object.insert(QStringLiteral("syntax"), syntaxObject);

    return object;
}

std::optional<ThemeDefinition> ThemeDefinition::fromJson(const QJsonObject &object)
{
    const QJsonObject paletteObject = object.value(QStringLiteral("palette")).toObject();
    const QJsonObject markdownObject = object.value(QStringLiteral("markdown")).toObject();
    if (paletteObject.isEmpty() || markdownObject.isEmpty())
        return std::nullopt;

    ThemeDefinition theme;
    theme.name = object.value(QStringLiteral("name")).toString();
    theme.syntaxThemeName = object.value(QStringLiteral("syntaxThemeName")).toString();
    if (theme.name.isEmpty() || theme.syntaxThemeName.isEmpty())
        return std::nullopt;

    theme.palette = makePalette(color(paletteObject, QStringLiteral("window")),
                                color(paletteObject, QStringLiteral("windowText")),
                                color(paletteObject, QStringLiteral("base")),
                                color(paletteObject, QStringLiteral("text")),
                                color(paletteObject, QStringLiteral("button")),
                                color(paletteObject, QStringLiteral("buttonText")),
                                color(paletteObject, QStringLiteral("highlight")),
                                color(paletteObject, QStringLiteral("highlightedText")),
                                color(paletteObject, QStringLiteral("link")),
                                color(paletteObject, QStringLiteral("linkVisited")),
                                color(paletteObject, QStringLiteral("placeholderText")));
    theme.palette.setColor(QPalette::AlternateBase, color(paletteObject, QStringLiteral("alternateBase")));

    theme.markdown.heading = color(markdownObject, QStringLiteral("heading"));
    theme.markdown.weakText = color(markdownObject, QStringLiteral("weakText"));
    theme.markdown.link = color(markdownObject, QStringLiteral("link"));
    theme.markdown.codeBackground = color(markdownObject, QStringLiteral("codeBackground"));
    theme.markdown.codeText = color(markdownObject, QStringLiteral("codeText"));
    theme.markdown.quoteBackground = color(markdownObject, QStringLiteral("quoteBackground"));
    theme.markdown.quoteBar = color(markdownObject, QStringLiteral("quoteBar"));
    theme.markdown.rule = color(markdownObject, QStringLiteral("rule"));
    theme.markdown.tableBorder = color(markdownObject, QStringLiteral("tableBorder"));
    theme.markdown.tableHeaderBackground = color(markdownObject, QStringLiteral("tableHeaderBackground"));
    theme.markdown.taskOpen = color(markdownObject, QStringLiteral("taskOpen"));
    theme.markdown.taskDone = color(markdownObject, QStringLiteral("taskDone"));

    // Syntax colors are optional in a theme file; when absent, derive them from
    // the palette's lightness so the code still fits the theme.
    const bool darkPalette = theme.palette.color(QPalette::Window).lightness() < 128;
    theme.syntax = darkPalette ? ThemeDefinition::dark().syntax : ThemeDefinition::light().syntax;

    const QJsonObject syntaxObject = object.value(QStringLiteral("syntax")).toObject();
    if (!syntaxObject.isEmpty()) {
        theme.syntax.keyword = color(syntaxObject, QStringLiteral("keyword"));
        theme.syntax.function = color(syntaxObject, QStringLiteral("function"));
        theme.syntax.variable = color(syntaxObject, QStringLiteral("variable"));
        theme.syntax.type = color(syntaxObject, QStringLiteral("type"));
        theme.syntax.constant = color(syntaxObject, QStringLiteral("constant"));
        theme.syntax.comment = color(syntaxObject, QStringLiteral("comment"));
        theme.syntax.string = color(syntaxObject, QStringLiteral("string"));
        theme.syntax.preprocessor = color(syntaxObject, QStringLiteral("preprocessor"));
        theme.syntax.op = color(syntaxObject, QStringLiteral("op"));
        theme.syntax.annotation = color(syntaxObject, QStringLiteral("annotation"));
        theme.syntax.warning = color(syntaxObject, QStringLiteral("warning"));
        theme.syntax.error = color(syntaxObject, QStringLiteral("error"));
    }

    // Reject malformed colour strings rather than silently applying an
    // invalid-builtin theme.
    const QVector<QColor> required = {
        theme.palette.color(QPalette::Window),
        theme.palette.color(QPalette::Text),
        theme.palette.color(QPalette::Highlight),
        theme.markdown.heading,
        theme.markdown.weakText,
        theme.markdown.link,
        theme.markdown.codeBackground,
        theme.markdown.codeText,
        theme.markdown.quoteBackground,
        theme.markdown.quoteBar,
        theme.markdown.rule,
        theme.markdown.tableBorder,
        theme.markdown.tableHeaderBackground,
        theme.markdown.taskOpen,
        theme.markdown.taskDone,
        theme.syntax.keyword,
        theme.syntax.function,
        theme.syntax.variable,
        theme.syntax.type,
        theme.syntax.constant,
        theme.syntax.comment,
        theme.syntax.string,
        theme.syntax.preprocessor,
        theme.syntax.op,
        theme.syntax.annotation,
        theme.syntax.warning,
        theme.syntax.error,
    };
    for (const QColor &value : required) {
        if (!value.isValid())
            return std::nullopt;
    }
    return theme;
}
