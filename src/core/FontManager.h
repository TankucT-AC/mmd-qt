// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFont>
#include <QStringList>

/*
 * Central typography for the whole application: markdown prose, headings and
 * code, with two interchangeable sets.
 *
 * - Bundled: Inter (OFL) for prose and UI, JetBrains Mono (OFL) for code,
 *   compiled into the executable as Qt resources, so the look is identical on
 *   every machine.
 * - System: the platform's proportional and fixed fonts.
 *
 * Heading sizes follow a modular type scale (base 12 pt, ratio 1.2), so the
 * hierarchy stays proportional when the base changes.
 */
class FontManager
{
public:
    enum class FontSet {
        Bundled,
        System,
    };

    // Registers the bundled OFL fonts with the application font database.
    // Idempotent. Returns the families that are available afterwards.
    static QStringList ensureBundledFontsLoaded();

    explicit FontManager(FontSet set = FontSet::Bundled);

    void setFontSet(FontSet set);
    FontSet fontSet() const;

    QFont bodyFont() const; // markdown prose
    QFont codeFont() const; // fenced blocks, inline code, source editor
    QFont headingFont(int level) const; // 1 (largest) .. 6 (smallest)

    qreal bodyPointSize() const;
    qreal codePointSize() const;
    static qreal headingPointSize(int level);

private:
    QFont proportionalFont(qreal pointSize, QFont::Weight weight) const;

    FontSet m_set = FontSet::Bundled;
};
