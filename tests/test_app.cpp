// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "app/MainWindow.h"

class AppTest : public QObject
{
    Q_OBJECT

private slots:
    void windowOpensWithBothPanelsVisible();
    void panelsCanBeClosedAndReopened();
    void themeChangeUpdatesApplicationPalette();
};

void AppTest::themeChangeUpdatesApplicationPalette()
{
    MainWindow window;
    window.themeManager()->setMode(ThemeManager::Mode::Dark);
    QCOMPARE(QApplication::palette().color(QPalette::Window), ThemeDefinition::dark().palette.color(QPalette::Window));

    window.themeManager()->setMode(ThemeManager::Mode::Light);
    QCOMPARE(QApplication::palette().color(QPalette::Window), ThemeDefinition::light().palette.color(QPalette::Window));
}

void AppTest::windowOpensWithBothPanelsVisible()
{
    MainWindow window;
    window.show();

    QVERIFY(window.editorPanel()->isVisible());
    QVERIFY(window.viewerPanel()->isVisible());
}

void AppTest::panelsCanBeClosedAndReopened()
{
    MainWindow window;
    window.show();

    window.toggleEditorAction()->trigger();
    QVERIFY(!window.editorPanel()->isVisible());
    QVERIFY(window.viewerPanel()->isVisible());

    window.toggleViewerAction()->trigger();
    QVERIFY(!window.viewerPanel()->isVisible());

    window.toggleEditorAction()->trigger();
    window.toggleViewerAction()->trigger();
    QVERIFY(window.editorPanel()->isVisible());
    QVERIFY(window.viewerPanel()->isVisible());
}

QTEST_MAIN(AppTest)
#include "test_app.moc"
