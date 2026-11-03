// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/SystemThemeProvider.h"
#include "core/Theme.h"
#include "core/ThemeManager.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonObject>
#include <QStyleHints>
#include <QTemporaryDir>

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
    void malformedColorIsRejected();
    void kdeSchemeIsParsed();
    void incompleteSchemeIsRejected();
    void currentLineIsDistinctFromEveryBackground();
    void currentLineIsOptionalInThemeFiles();

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
                                    colors.taskDone,
                                    colors.currentLine}) {
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
    if (SystemThemeProvider::currentSystemTheme().has_value())
        QSKIP("A desktop colour scheme is present; System mode follows it instead of QStyleHints");
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
    QCOMPARE(restored->markdown.currentLine, original.markdown.currentLine);
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

void ThemeTest::malformedColorIsRejected()
{
    QJsonObject json = ThemeDefinition::light().toJson();
    QJsonObject markdown = json.value(QStringLiteral("markdown")).toObject();
    markdown.insert(QStringLiteral("link"), QStringLiteral("not-a-color"));
    json.insert(QStringLiteral("markdown"), markdown);

    QVERIFY(!ThemeDefinition::fromJson(json).has_value());
}

void ThemeTest::currentLineIsDistinctFromEveryBackground()
{
    for (const ThemeDefinition &theme : {ThemeDefinition::light(), ThemeDefinition::dark()}) {
        QVERIFY2(theme.markdown.currentLine.isValid(), qPrintable(theme.name + QStringLiteral(": currentLine unset")));
        // It used to share codeBackground, so the active line vanished inside a
        // fenced block. It must stand out on the plain background too.
        QVERIFY2(theme.markdown.currentLine != theme.markdown.codeBackground, qPrintable(theme.name + QStringLiteral(": currentLine equals codeBackground")));
        QVERIFY2(theme.markdown.currentLine != theme.palette.color(QPalette::Base), qPrintable(theme.name + QStringLiteral(": currentLine equals the base")));
        QVERIFY2(theme.markdown.currentLine != theme.palette.color(QPalette::Window),
                 qPrintable(theme.name + QStringLiteral(": currentLine equals the window")));
    }
}

void ThemeTest::currentLineIsOptionalInThemeFiles()
{
    // Themes written before the role existed must still load.
    QJsonObject json = ThemeDefinition::light().toJson();
    QJsonObject markdown = json.value(QStringLiteral("markdown")).toObject();
    markdown.remove(QStringLiteral("currentLine"));
    json.insert(QStringLiteral("markdown"), markdown);

    const std::optional<ThemeDefinition> restored = ThemeDefinition::fromJson(json);
    QVERIFY(restored.has_value());
    QVERIFY(restored->markdown.currentLine.isValid());
    QVERIFY(restored->markdown.currentLine != restored->markdown.codeBackground);
}

void ThemeTest::kdeSchemeIsParsed()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("TestScheme.colors"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(
        "[General]\nName=TestScheme\nColorScheme=TestScheme\n\n"
        "[Colors:Window]\nBackgroundNormal=30,32,40\nForegroundNormal=230,237,243\n"
        "ForegroundInactive=139,148,158\nForegroundLink=88,166,255\nForegroundVisited=210,168,255\n\n"
        "[Colors:View]\nBackgroundNormal=28,32,40\nForegroundNormal=230,237,243\n\n"
        "[Colors:Button]\nBackgroundNormal=38,43,53\nForegroundNormal=230,237,243\n\n"
        "[Colors:Selection]\nBackgroundNormal=88,166,255\nForegroundNormal=255,255,255\n");
    file.close();

    const std::optional<ThemeDefinition> theme = SystemThemeProvider::loadScheme(path);
    QVERIFY(theme.has_value());
    QCOMPARE(theme->name, QStringLiteral("TestScheme"));
    QCOMPARE(theme->palette.color(QPalette::Window), QColor(30, 32, 40));
    QCOMPARE(theme->palette.color(QPalette::Link), QColor(88, 166, 255));
    // A dark scheme derives the dark syntax palette.
    QCOMPARE(theme->syntax.keyword, ThemeDefinition::dark().syntax.keyword);
}

void ThemeTest::incompleteSchemeIsRejected()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("Empty.colors"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("[General]\nName=Empty\n");
    file.close();

    QVERIFY(!SystemThemeProvider::loadScheme(path).has_value());
}

QTEST_MAIN(ThemeTest)
#include "test_theme.moc"
