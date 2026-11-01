// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/FontManager.h"

#include <QFontDatabase>

// The font resources live in the static mmd_core library; the linker would
// drop the generated resource initializer unless something references it.
// Q_INIT_RESOURCE must be called from a function outside any namespace.
static void initFontResources()
{
    Q_INIT_RESOURCE(fonts);
}

namespace
{

// Modular type scale: base 12 pt, minor third (1.2). H4 keeps the body size
// and distinguishes itself by weight; H6 steps down for meta-level headings.
constexpr qreal kBodyPointSize = 12.0;
constexpr qreal kCodePointSize = 10.5;
constexpr qreal kScaleRatio = 1.2;

constexpr int kHeadingCount = 6;

QFont::Weight headingWeight(int level)
{
    switch (level) {
    case 1:
    case 2:
        return QFont::Bold;
    case 3:
    case 4:
        return QFont::DemiBold;
    case 5:
        return QFont::Medium;
    default:
        return QFont::Normal;
    }
}

} // namespace

QStringList FontManager::ensureBundledFontsLoaded()
{
    static QStringList families;
    static bool loaded = false;
    if (loaded)
        return families;

    initFontResources();

    const QFontDatabase database;

    const QStringList resources = {
        QStringLiteral(":/mmd/fonts/Inter-Regular.ttf"),
        QStringLiteral(":/mmd/fonts/Inter-Italic.ttf"),
        QStringLiteral(":/mmd/fonts/Inter-Medium.ttf"),
        QStringLiteral(":/mmd/fonts/Inter-SemiBold.ttf"),
        QStringLiteral(":/mmd/fonts/Inter-Bold.ttf"),
        QStringLiteral(":/mmd/fonts/JetBrainsMono-Regular.ttf"),
        QStringLiteral(":/mmd/fonts/JetBrainsMono-Italic.ttf"),
        QStringLiteral(":/mmd/fonts/JetBrainsMono-Bold.ttf"),
        QStringLiteral(":/mmd/fonts/JetBrainsMono-BoldItalic.ttf"),
    };
    for (const QString &path : resources) {
        const int id = QFontDatabase::addApplicationFont(path);
        if (id < 0)
            qWarning("mmd: failed to load bundled font %s", qPrintable(path));
    }

    loaded = true;
    families.clear();
    const QStringList after = database.families();
    for (const QString &family : after) {
        if (family == QStringLiteral("Inter") || family == QStringLiteral("JetBrains Mono"))
            families.append(family);
    }
    return families;
}

FontManager::FontManager(FontSet set)
    : m_set(set)
{
}

void FontManager::setFontSet(FontSet set)
{
    m_set = set;
}

FontManager::FontSet FontManager::fontSet() const
{
    return m_set;
}

QFont FontManager::bodyFont() const
{
    return proportionalFont(bodyPointSize(), QFont::Normal);
}

QFont FontManager::codeFont() const
{
    QFont font;
    if (m_set == FontSet::Bundled)
        font.setFamily(QStringLiteral("JetBrains Mono"));
    else
        font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    font.setPointSizeF(codePointSize());
    return font;
}

QFont FontManager::headingFont(int level) const
{
    const int clamped = qBound(1, level, kHeadingCount);
    return proportionalFont(headingPointSize(clamped), headingWeight(clamped));
}

qreal FontManager::bodyPointSize() const
{
    return kBodyPointSize;
}

qreal FontManager::codePointSize() const
{
    return kCodePointSize;
}

qreal FontManager::headingPointSize(int level)
{
    switch (qBound(1, level, kHeadingCount)) {
    case 1:
        return kBodyPointSize * kScaleRatio * kScaleRatio * kScaleRatio;
    case 2:
        return kBodyPointSize * kScaleRatio * kScaleRatio;
    case 3:
        return kBodyPointSize * kScaleRatio;
    case 4:
    case 5:
        return kBodyPointSize;
    default:
        return kBodyPointSize / kScaleRatio;
    }
}

QFont FontManager::proportionalFont(qreal pointSize, QFont::Weight weight) const
{
    QFont font;
    if (m_set == FontSet::Bundled)
        font.setFamily(QStringLiteral("Inter"));
    else
        font = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    font.setPointSizeF(pointSize);
    font.setWeight(weight);
    return font;
}
