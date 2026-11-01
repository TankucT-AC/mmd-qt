// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "app/FileTreePanel.h"
#include "app/MainWindow.h"
#include "app/OutlinePanel.h"
#include "editor/MarkdownEditor.h"
#include "viewer/MarkdownView.h"

#include <QFile>
#include <QFileSystemModel>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTreeView>
#include <QTreeWidget>

class SidebarTest : public QObject
{
    Q_OBJECT

private slots:
    void outlineNestsByLevel();
    void activatingARowReportsItsIndex();
    void currentBlockSelectsTheMatchingRow();
    void fileTreeListsOnlyMarkdown();
    void fileTreeOpensTheClickedFile();
    void windowOutlineFollowsTheDocument();
    void outlineClickScrollsThePreview();
};

void SidebarTest::outlineNestsByLevel()
{
    OutlinePanel panel;
    panel.setOutline({
        {1, QStringLiteral("One"), QStringLiteral("one"), 0},
        {2, QStringLiteral("Two"), QStringLiteral("two"), 4},
        {2, QStringLiteral("Three"), QStringLiteral("three"), 8},
        {1, QStringLiteral("Four"), QStringLiteral("four"), 12},
    });

    auto *tree = panel.findChild<QTreeWidget *>();
    QVERIFY(tree);
    QCOMPARE(tree->topLevelItemCount(), 2); // levels 1 are the roots
    QCOMPARE(tree->topLevelItem(0)->text(0), QStringLiteral("One"));
    QCOMPARE(tree->topLevelItem(0)->childCount(), 2); // the two level-2 headings
    QCOMPARE(tree->topLevelItem(1)->text(0), QStringLiteral("Four"));
    QCOMPARE(panel.entryCount(), 4);
}

void SidebarTest::activatingARowReportsItsIndex()
{
    OutlinePanel panel;
    panel.setOutline({
        {1, QStringLiteral("One"), QStringLiteral("one"), 0},
        {2, QStringLiteral("Two"), QStringLiteral("two"), 4},
    });

    auto *tree = panel.findChild<QTreeWidget *>();
    QVERIFY(tree);

    QSignalSpy spy(&panel, &OutlinePanel::entryActivated);
    tree->itemClicked(tree->topLevelItem(0), 0);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toInt(), 0);

    tree->itemClicked(tree->topLevelItem(0)->child(0), 0);
    QCOMPARE(spy.count(), 2);
    QCOMPARE(spy.at(1).at(0).toInt(), 1);
}

void SidebarTest::currentBlockSelectsTheMatchingRow()
{
    OutlinePanel panel;
    panel.setOutline({
        {1, QStringLiteral("One"), QStringLiteral("one"), 0},
        {2, QStringLiteral("Two"), QStringLiteral("two"), 4},
    });

    auto *tree = panel.findChild<QTreeWidget *>();
    QVERIFY(tree);

    panel.setCurrentBlock(4);
    QVERIFY(tree->currentItem());
    QCOMPARE(tree->currentItem()->text(0), QStringLiteral("Two"));

    // A block number that is not in the outline clears the mark rather than
    // leaving a stale row selected.
    panel.setCurrentBlock(999);
    QVERIFY(!tree->currentItem());
}

void SidebarTest::fileTreeListsOnlyMarkdown()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QStringList names = {
        QStringLiteral("a.md"),
        QStringLiteral("b.markdown"),
        QStringLiteral("notes.txt"),
        QStringLiteral("picture.png"),
    };
    for (const QString &name : names) {
        QFile file(dir.filePath(name));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x");
    }

    FileTreePanel panel;
    panel.setRootDirectory(dir.path());
    QCOMPARE(panel.rootDirectory(), dir.path());

    auto *view = panel.findChild<QTreeView *>();
    QVERIFY(view);

    // QFileSystemModel fills in asynchronously, so let the event loop run.
    QTRY_COMPARE_WITH_TIMEOUT(view->model()->rowCount(view->rootIndex()), 2, 5000);

    QStringList listed;
    for (int row = 0; row < view->model()->rowCount(view->rootIndex()); ++row)
        listed << view->model()->index(row, 0, view->rootIndex()).data(QFileSystemModel::FileNameRole).toString();
    listed.sort();
    QCOMPARE(listed, QStringList({QStringLiteral("a.md"), QStringLiteral("b.markdown")}));
}

void SidebarTest::fileTreeOpensTheClickedFile()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath(QStringLiteral("chapter.md")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("# Chapter\n");

    FileTreePanel panel;
    panel.setRootDirectory(dir.path());

    auto *view = panel.findChild<QTreeView *>();
    QVERIFY(view);
    QTRY_COMPARE_WITH_TIMEOUT(view->model()->rowCount(view->rootIndex()), 1, 5000);

    QSignalSpy spy(&panel, &FileTreePanel::fileActivated);
    view->clicked(view->model()->index(0, 0, view->rootIndex()));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).toString(), dir.filePath(QStringLiteral("chapter.md")));
}

void SidebarTest::windowOutlineFollowsTheDocument()
{
    MainWindow window;
    window.viewer()->setRenderInterval(0);
    window.editor()->setMarkdown(QStringLiteral("# One\n\nbody\n\n## Two\n\nmore\n"));

    auto *panel = window.findChild<OutlinePanel *>();
    QVERIFY(panel);
    QCOMPARE(panel->entryCount(), 2);

    auto *tree = panel->findChild<QTreeWidget *>();
    QVERIFY(tree);
    QCOMPARE(tree->topLevelItemCount(), 1);
    QCOMPARE(tree->topLevelItem(0)->childCount(), 1);

    // Editing the headings updates the panel without touching anything else.
    window.editor()->setMarkdown(QStringLiteral("# Only\n"));
    QCOMPARE(panel->entryCount(), 1);
}

void SidebarTest::outlineClickScrollsThePreview()
{
    MainWindow window;
    window.resize(900, 400);
    window.show();
    window.viewer()->setRenderInterval(0);

    QString markdown = QStringLiteral("# Intro\n\n");
    for (int i = 0; i < 120; ++i)
        markdown += QStringLiteral("line %1\n\n").arg(i);
    markdown += QStringLiteral("# Key features\n\nbody\n");
    window.editor()->setMarkdown(markdown);
    window.viewer()->renderNow();

    auto *panel = window.findChild<OutlinePanel *>();
    QVERIFY(panel);
    QCOMPARE(panel->entryCount(), 2);

    auto *tree = panel->findChild<QTreeWidget *>();
    QVERIFY(tree);
    const int before = window.viewer()->verticalScrollBar()->value();

    tree->itemClicked(tree->topLevelItem(1), 0); // "Key features"
    QVERIFY(window.viewer()->verticalScrollBar()->value() > before);
    // The section the reader is now under is marked in the outline.
    QCOMPARE(tree->currentItem()->text(0), QStringLiteral("Key features"));
}

QTEST_MAIN(SidebarTest)
#include "test_sidebar.moc"
