// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/FontManager.h"
#include "core/Theme.h"

#include <QTextFormat>

class QString;
class QTextDocument;

/*
 * Everything the markdown renderer needs to know about looks: colors, fonts
 * and the base directory for resolving relative image references.
 */
struct RenderStyle {
    ThemeDefinition theme;
    FontManager fonts;
    QString baseDir;
    int maxImageWidth = 760;
};

/*
 * Turns markdown source into a rendered QTextDocument using the md4c parser
 * (the same parser Qt's own markdown import is based on).
 *
 * The builder is a pure function of (source, style): no widgets, no event
 * loop, safe to call from tests. Rebuilding is done in place — clear() plus
 * appending — so the QTextDocument instance and its layout stay valid across
 * incremental re-renders.
 *
 * Blocks are marked with properties so later passes can find them without
 * re-parsing: fenced code blocks carry their language in languageKey, which
 * is what the syntax highlighter keys on.
 */
class MarkdownBuilder
{
public:
    enum class BlockKind {
        None = 0,
        Heading,
        Code,
        Quote,
        Rule,
    };

    // QTextFormat property keys used on block formats and the GIF object.
    enum PropertyKey : int {
        kindKey = QTextFormat::UserProperty + 1,
        headingLevelKey,
        languageKey,
        gifPathKey,
        checkboxCheckedKey,
        checkboxSizeKey,
    };

    // QTextCharFormat object type used for animated images. The viewer
    // registers a QTextObjectInterface handler for it.
    static constexpr int gifObjectType = QTextFormat::UserObject + 1;
    // Object type used for task-list checkboxes; the viewer draws them.
    static constexpr int checkboxObjectType = QTextFormat::UserObject + 2;

    // Parses markdown and rebuilds target as a rendered document.
    // Returns false only when the parser itself fails.
    static bool build(QTextDocument *target, const QString &markdown, const RenderStyle &style);
};
