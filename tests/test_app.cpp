// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "app/MainWindow.h"
#include "editor/MarkdownEditor.h"
#include "viewer/MarkdownView.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTextCursor>

class AppTest : public QObject
{
    Q_OBJECT

private slots:
    void windowOpensWithBothPanelsVisible();
    void panelsCanBeClosedAndReopened();
    void themeChangeUpdatesApplicationPalette();
    void typingUpdatesPreview();
    void openingFileLoadsContentAndTitle();
    void savingFileWritesToDisk();
    void externalChangeReloadsUnmodifiedDocument();

private:
    static void typeInto(MarkdownEditor *editor, const QString &text, QTextCursor::MoveMode mode = QTextCursor::MoveAnchor);
};

void AppTest::typeInto(MarkdownEditor *editor, const QString &text, QTextCursor::MoveMode)
{
    QTextCursor cursor = editor->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
}

void AppTest::typingUpdatesPreview()
{
    MainWindow window;
    window.viewer()->setRenderInterval(0);

    typeInto(window.editor(), QStringLiteral("# Hello\n\nworld\n"));

    const QString text = window.viewer()->document()->toPlainText();
    QVERIFY(text.contains(QStringLiteral("Hello")));
    QVERIFY(text.contains(QStringLiteral("world")));
}

void AppTest::openingFileLoadsContentAndTitle()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("note.md"));
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("# From disk\n");
    }

    MainWindow window;
    window.viewer()->setRenderInterval(0);
    QVERIFY(window.openFile(path));

    QCOMPARE(window.editor()->toPlainText(), QStringLiteral("# From disk\n"));
    QVERIFY(window.viewer()->document()->toPlainText().contains(QStringLiteral("From disk")));
    QVERIFY(window.windowTitle().contains(QStringLiteral("note.md")));
    QVERIFY(!window.windowTitle().startsWith(QLatin1Char('*')));
}

void AppTest::savingFileWritesToDisk()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("save.md"));
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("initial\n");
    }

    MainWindow window;
    window.viewer()->setRenderInterval(0);
    QVERIFY(window.openFile(path)); // remembers the file path
    window.editor()->setMarkdown(QStringLiteral("written\n"));
    window.editor()->document()->setModified(true);
    QVERIFY(window.saveFile());

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(QString::fromUtf8(file.readAll()), QStringLiteral("written\n"));
    QVERIFY(!window.editor()->document()->isModified());
}

void AppTest::externalChangeReloadsUnmodifiedDocument()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("live.md"));
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("first\n");
    }

    MainWindow window;
    window.viewer()->setRenderInterval(0);
    QVERIFY(window.openFile(path));
    QVERIFY(!window.editor()->document()->isModified());

    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
        file.write("second\n");
    }

    QTRY_VERIFY_WITH_TIMEOUT(window.editor()->toPlainText() == QStringLiteral("second\n"), 3000);
    QVERIFY(window.viewer()->document()->toPlainText().contains(QStringLiteral("second")));
}

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
