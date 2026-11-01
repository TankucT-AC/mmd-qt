// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/Theme.h"

#include <QJsonArray>

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
    return theme;
}
