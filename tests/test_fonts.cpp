// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "app/FontsDialog.h"
#include "core/FontManager.h"

#include <QFontDatabase>
#include <QPushButton>

class FontTest : public QObject
{
    Q_OBJECT

private slots:
    void bundledFontsAreAvailable();
    void bundledSetUsesBundledFamilies();
    void systemSetUsesSystemFamilies();
    void customSetUsesChosenFamilies();
    void customSetFallsBackToBundledWhenBlank();
    void headingsFormMonotonicScale();
    void codeFontIsMonospaced();
    void dialogRoundTripsFamilies();
    void dialogResetRestoresBundledFamilies();
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

void FontTest::customSetUsesChosenFamilies()
{
    FontManager manager(FontManager::FontSet::Custom);
    manager.setCustomFonts(QStringLiteral("Georgia"), QStringLiteral("Courier New"));
    QCOMPARE(manager.bodyFont().family(), QStringLiteral("Georgia"));
    QCOMPARE(manager.codeFont().family(), QStringLiteral("Courier New"));
    QCOMPARE(manager.headingFont(1).family(), QStringLiteral("Georgia"));
    // The type scale is independent of the family.
    QCOMPARE(manager.bodyFont().pointSizeF(), manager.bodyPointSize());
    QCOMPARE(manager.codeFont().pointSizeF(), manager.codePointSize());
}

void FontTest::customSetFallsBackToBundledWhenBlank()
{
    FontManager manager(FontManager::FontSet::Custom);
    manager.setCustomFonts(QString(), QString());
    QCOMPARE(manager.bodyFont().family(), QStringLiteral("Inter"));
    QCOMPARE(manager.codeFont().family(), QStringLiteral("JetBrains Mono"));

    // Each member falls back on its own, so a custom code font can pair with
    // the bundled text font.
    manager.setCustomFonts(QString(), QStringLiteral("Courier New"));
    QCOMPARE(manager.bodyFont().family(), QStringLiteral("Inter"));
    QCOMPARE(manager.codeFont().family(), QStringLiteral("Courier New"));
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

void FontTest::dialogRoundTripsFamilies()
{
    FontsDialog dialog;
    // The combos come up with a selectable family, not empty.
    QVERIFY(!dialog.bodyFamily().isEmpty());
    QVERIFY(!dialog.codeFamily().isEmpty());

    // The bundled faces are registered application fonts, so they are
    // selectable like any other installed family.
    dialog.setBodyFamily(QStringLiteral("Inter"));
    QCOMPARE(dialog.bodyFamily(), QStringLiteral("Inter"));
}

void FontTest::dialogResetRestoresBundledFamilies()
{
    FontsDialog dialog;
    auto *reset = dialog.findChild<QPushButton *>(QStringLiteral("fontsResetButton"));
    QVERIFY(reset);

    QTest::mouseClick(reset, Qt::LeftButton);
    QCOMPARE(dialog.bodyFamily(), QStringLiteral("Inter"));
    QCOMPARE(dialog.codeFamily(), QStringLiteral("JetBrains Mono"));
}

QTEST_MAIN(FontTest)
#include "test_fonts.moc"
