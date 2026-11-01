// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/FontManager.h"

#include <QFontDatabase>

class FontTest : public QObject
{
    Q_OBJECT

private slots:
    void bundledFontsAreAvailable();
    void bundledSetUsesBundledFamilies();
    void systemSetUsesSystemFamilies();
    void headingsFormMonotonicScale();
    void codeFontIsMonospaced();
};

void FontTest::bundledFontsAreAvailable()
{
    const QStringList families = FontManager::ensureBundledFontsLoaded();
    QVERIFY2(families.contains(QStringLiteral("Inter"), Qt::CaseInsensitive), "Inter must load from resources");
    QVERIFY2(families.contains(QStringLiteral("JetBrains Mono"), Qt::CaseInsensitive), "JetBrains Mono must load from resources");
}

void FontTest::bundledSetUsesBundledFamilies()
{
    FontManager manager(FontManager::FontSet::Bundled);
    QCOMPARE(manager.bodyFont().family(), QStringLiteral("Inter"));
    QCOMPARE(manager.codeFont().family(), QStringLiteral("JetBrains Mono"));
    QCOMPARE(manager.headingFont(1).family(), QStringLiteral("Inter"));
}

void FontTest::systemSetUsesSystemFamilies()
{
    FontManager manager(FontManager::FontSet::System);
    const QFont expectedBody = QFontDatabase::systemFont(QFontDatabase::GeneralFont);
    const QFont expectedCode = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    QCOMPARE(manager.bodyFont().family(), expectedBody.family());
    QCOMPARE(manager.codeFont().family(), expectedCode.family());
}

void FontTest::headingsFormMonotonicScale()
{
    FontManager manager;
    QVERIFY(manager.headingFont(1).pointSizeF() > manager.headingFont(2).pointSizeF());
    QVERIFY(manager.headingFont(2).pointSizeF() > manager.headingFont(3).pointSizeF());
    QVERIFY(manager.headingFont(3).pointSizeF() >= manager.bodyFont().pointSizeF());
    QVERIFY(manager.headingFont(6).pointSizeF() <= manager.bodyFont().pointSizeF());
}

void FontTest::codeFontIsMonospaced()
{
    FontManager manager;
    const QFontInfo info(manager.codeFont());
    QVERIFY(info.fixedPitch());
    QVERIFY(manager.codeFont().pointSizeF() < manager.bodyFont().pointSizeF());
}

QTEST_MAIN(FontTest)
#include "test_fonts.moc"
