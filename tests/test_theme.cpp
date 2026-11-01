// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/Theme.h"
#include "core/ThemeManager.h"

#include <QGuiApplication>
#include <QStyleHints>

class ThemeTest : public QObject
{
    Q_OBJECT

private slots:
    void builtinThemesAreComplete();
    void lightAndDarkDiffer();
    void defaultModeIsSystem();
    void explicitModesDoNotFollowPlatform();
    void systemModeFollowsPlatformScheme();
    void modeChangeEmitsThemeChanged();
    void customThemeRoundTripsThroughJson();
    void customModeUsesProvidedDefinition();

private:
    static bool setPlatformScheme(Qt::ColorScheme scheme);
};

bool ThemeTest::setPlatformScheme(Qt::ColorScheme scheme)
{
    QGuiApplication::styleHints()->setColorScheme(scheme);
    return QGuiApplication::styleHints()->colorScheme() == scheme;
}

void ThemeTest::builtinThemesAreComplete()
{
    for (const ThemeDefinition &theme : {ThemeDefinition::light(), ThemeDefinition::dark()}) {
        QVERIFY(!theme.name.isEmpty());
        QVERIFY(!theme.syntaxThemeName.isEmpty());
        QVERIFY(theme.palette.color(QPalette::WindowText).isValid());
        QVERIFY(theme.palette.color(QPalette::Text).isValid());
        QVERIFY(theme.palette.color(QPalette::Highlight).isValid());
        const auto colors = theme.markdown;
        for (const QColor &color : {colors.heading,
                                    colors.weakText,
                                    colors.link,
                                    colors.codeBackground,
                                    colors.codeText,
                                    colors.quoteBackground,
                                    colors.quoteBar,
                                    colors.rule,
                                    colors.tableBorder,
                                    colors.tableHeaderBackground,
                                    colors.taskOpen,
                                    colors.taskDone}) {
            QVERIFY2(color.isValid(), qPrintable(theme.name + QStringLiteral(": unset markdown color")));
        }
    }
}

void ThemeTest::lightAndDarkDiffer()
{
    const ThemeDefinition light = ThemeDefinition::light();
    const ThemeDefinition dark = ThemeDefinition::dark();
    QVERIFY(light.palette.color(QPalette::Window) != dark.palette.color(QPalette::Window));
    QVERIFY(light.markdown.link != dark.markdown.link);
    QVERIFY(light.syntaxThemeName != dark.syntaxThemeName);
}

void ThemeTest::defaultModeIsSystem()
{
    ThemeManager manager;
    QCOMPARE(manager.mode(), ThemeManager::Mode::System);
}

void ThemeTest::explicitModesDoNotFollowPlatform()
{
    if (!setPlatformScheme(Qt::ColorScheme::Dark))
        QSKIP("This platform cannot override the color scheme");

    ThemeManager manager;
    manager.setMode(ThemeManager::Mode::Light);
    QCOMPARE(manager.currentTheme().name, QStringLiteral("light"));
    QVERIFY(setPlatformScheme(Qt::ColorScheme::Light));
    QCOMPARE(manager.currentTheme().name, QStringLiteral("light"));
    setPlatformScheme(Qt::ColorScheme::Unknown);
}

void ThemeTest::systemModeFollowsPlatformScheme()
{
    if (!setPlatformScheme(Qt::ColorScheme::Dark))
        QSKIP("This platform cannot override the color scheme");

    ThemeManager manager;
    QCOMPARE(manager.mode(), ThemeManager::Mode::System);

    QTRY_COMPARE(manager.currentTheme().name, QStringLiteral("dark"));
    setPlatformScheme(Qt::ColorScheme::Light);
    QTRY_COMPARE(manager.currentTheme().name, QStringLiteral("light"));
    setPlatformScheme(Qt::ColorScheme::Unknown);
}

void ThemeTest::modeChangeEmitsThemeChanged()
{
    ThemeManager manager;
    QSignalSpy spy(&manager, &ThemeManager::themeChanged);
    manager.setMode(ThemeManager::Mode::Dark);
    QCOMPARE(spy.count(), 1);
    // Setting the same mode again must not re-emit.
    manager.setMode(ThemeManager::Mode::Dark);
    QCOMPARE(spy.count(), 1);
}

void ThemeTest::customThemeRoundTripsThroughJson()
{
    ThemeDefinition original = ThemeDefinition::dark();
    original.name = QStringLiteral("custom-test");

    const QJsonObject json = original.toJson();
    const std::optional<ThemeDefinition> restored = ThemeDefinition::fromJson(json);

    QVERIFY(restored.has_value());
    QCOMPARE(restored->name, original.name);
    QCOMPARE(restored->syntaxThemeName, original.syntaxThemeName);
    QCOMPARE(restored->markdown.link, original.markdown.link);
    QCOMPARE(restored->palette.color(QPalette::Window), original.palette.color(QPalette::Window));
}

void ThemeTest::customModeUsesProvidedDefinition()
{
    ThemeManager manager;
    ThemeDefinition custom = ThemeDefinition::light();
    custom.name = QStringLiteral("my-theme");

    manager.setCustomTheme(custom);
    manager.setMode(ThemeManager::Mode::Custom);

    QCOMPARE(manager.currentTheme().name, QStringLiteral("my-theme"));
}

QTEST_MAIN(ThemeTest)
#include "test_theme.moc"
