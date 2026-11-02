// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "app/FileTreePanel.h"
#include "app/MainWindow.h"
#include "app/OutlinePanel.h"
#include "core/Outline.h"
#include "editor/MarkdownEditor.h"
#include "viewer/MarkdownView.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QFile>
#include <QFileSystemModel>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextCursor>
#include <QTreeView>
#include <QTreeWidget>

#include <functional>

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
    void outlineClickLandsOnTheClickedSection();
    void outlineClickKeepsTheBarOnTheClickedRowNearTheEnd();
    void outlineFollowsTheReadingAgainAfterUserScroll();
    void clickingThePreviewMovesTheEditorCaret();
    void theSidebarSeparatorIsAHairline();
    void theSidebarTitleBarHasUsableButtons();
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

void SidebarTest::outlineClickLandsOnTheClickedSection()
{
    MainWindow window;
    window.resize(900, 400);
    window.show();
    window.viewer()->setRenderInterval(0);

    // A document whose level-1 heading sits in the middle, the way a real one
    // does: the tree nests everything after it underneath, and a click on a row
    // has to land on that row's own heading -- not the next one, which is what
    // the (now removed) proportional scroll synchroniser used to cause.
    QString markdown = QStringLiteral("# Introduction\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("filler %1\n\n").arg(i);
    markdown += QStringLiteral("# Heading level 1\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("more filler %1\n\n").arg(i);
    markdown += QStringLiteral("## Section two\n\nbody\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("tail %1\n\n").arg(i);
    markdown += QStringLiteral("## Section three\n\nbody\n");

    window.editor()->setMarkdown(markdown);
    window.viewer()->renderNow();

    auto *panel = window.findChild<OutlinePanel *>();
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeWidget *>();
    QVERIFY(tree);

    QList<QTreeWidgetItem *> rows;
    std::function<void(QTreeWidgetItem *)> collect = [&](QTreeWidgetItem *item) {
        for (int child = 0; child < item->childCount(); ++child) {
            rows.append(item->child(child));
            collect(item->child(child));
        }
    };
    for (int i = 0; i < tree->topLevelItemCount(); ++i) {
        rows.append(tree->topLevelItem(i));
        collect(tree->topLevelItem(i));
    }
    QCOMPARE(rows.size(), 4);

    const QVector<OutlineEntry> entries = Outline::fromDocument(*window.viewer()->document());
    const QVector<int> sourceLines = Outline::headingSourceLines(markdown);
    QCOMPARE(entries.size(), 4);
    QCOMPARE(sourceLines.size(), 4);

    for (QTreeWidgetItem *row : rows) {
        const int index = row->data(0, Qt::UserRole).toInt();
        const QRect rect = tree->visualItemRect(row);
        QVERIFY(!rect.isNull());
        QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, rect.center());

        QCOMPARE(window.viewer()->visibleHeading(), entries.at(index).blockNumber);
        QCOMPARE(window.editor()->textCursor().block().blockNumber(), sourceLines.at(index));
    }
}

// A section within one viewport of the document end cannot be brought to the
// reading line: the scroll clamps at the bottom. The bar has to stay on the
// clicked row instead of sliding to whatever ends up under the reading line
// (the last heading on screen) -- this is the "click 10, land on 11" bug.
void SidebarTest::outlineClickKeepsTheBarOnTheClickedRowNearTheEnd()
{
    MainWindow window;
    window.resize(900, 400);
    window.show();
    window.viewer()->setRenderInterval(0);

    QString markdown = QStringLiteral("# Introduction\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("filler %1\n\n").arg(i);
    markdown += QStringLiteral("# Section one\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("more filler %1\n\n").arg(i);
    markdown += QStringLiteral("## Section two\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("tail %1\n\n").arg(i);
    markdown += QStringLiteral("## Near end\n\nbody\n\n");
    markdown += QStringLiteral("## The last one\n\nbody\n");

    window.editor()->setMarkdown(markdown);
    window.viewer()->renderNow();

    auto *panel = window.findChild<OutlinePanel *>();
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeWidget *>();
    QVERIFY(tree);

    QTreeWidgetItem *nearEnd = nullptr;
    std::function<void(QTreeWidgetItem *)> find = [&](QTreeWidgetItem *item) {
        if (item->text(0) == QLatin1String("Near end"))
            nearEnd = item;
        for (int child = 0; child < item->childCount(); ++child)
            find(item->child(child));
    };
    for (int i = 0; i < tree->topLevelItemCount(); ++i)
        find(tree->topLevelItem(i));
    QVERIFY(nearEnd);

    const int nearEndIndex = nearEnd->data(0, Qt::UserRole).toInt();
    const QVector<OutlineEntry> entries = Outline::fromDocument(*window.viewer()->document());
    QVERIFY(!tree->visualItemRect(nearEnd).isNull());
    QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, tree->visualItemRect(nearEnd).center());

    QCOMPARE(window.viewer()->visibleHeading(), entries.at(nearEndIndex).blockNumber);
    QVERIFY2(tree->currentItem() == nearEnd, "the outline bar must stay on the clicked row");
}

void SidebarTest::outlineFollowsTheReadingAgainAfterUserScroll()
{
    MainWindow window;
    window.resize(900, 400);
    window.show();
    window.viewer()->setRenderInterval(0);

    QString markdown = QStringLiteral("# Introduction\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("filler %1\n\n").arg(i);
    markdown += QStringLiteral("# Section one\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("more filler %1\n\n").arg(i);
    markdown += QStringLiteral("## Section two\n\nbody\n\n");
    for (int i = 0; i < 40; ++i)
        markdown += QStringLiteral("tail %1\n\n").arg(i);

    window.editor()->setMarkdown(markdown);
    window.viewer()->renderNow();

    auto *panel = window.findChild<OutlinePanel *>();
    QVERIFY(panel);
    auto *tree = panel->findChild<QTreeWidget *>();
    QVERIFY(tree);

    QTreeWidgetItem *sectionTwo = nullptr;
    std::function<void(QTreeWidgetItem *)> find = [&](QTreeWidgetItem *item) {
        if (item->text(0) == QLatin1String("Section two"))
            sectionTwo = item;
        for (int child = 0; child < item->childCount(); ++child)
            find(item->child(child));
    };
    for (int i = 0; i < tree->topLevelItemCount(); ++i)
        find(tree->topLevelItem(i));
    QVERIFY(sectionTwo);

    // The click pins the bar to the clicked row...
    QTest::mouseClick(tree->viewport(), Qt::LeftButton, Qt::NoModifier, tree->visualItemRect(sectionTwo).center());
    QCOMPARE(tree->currentItem(), sectionTwo);

    // ...and a user scroll hands the bar back to the reading position.
    window.viewer()->setFocus();
    QTest::keyClick(window.viewer(), Qt::Key_PageUp);
    QVERIFY(tree->currentItem() != sectionTwo);

    const QVector<OutlineEntry> entries = Outline::fromDocument(*window.viewer()->document());
    const int visible = window.viewer()->visibleHeading();
    int visibleIndex = -1;
    for (int i = 0; i < entries.size(); ++i) {
        if (entries.at(i).blockNumber == visible)
            visibleIndex = i;
    }
    QVERIFY(visibleIndex >= 0);
    QCOMPARE(tree->currentItem()->data(0, Qt::UserRole).toInt(), visibleIndex);
}

void SidebarTest::clickingThePreviewMovesTheEditorCaret()
{
    MainWindow window;
    window.resize(900, 500);
    window.show();
    window.viewer()->setRenderInterval(0);
    window.editor()->setMarkdown(QStringLiteral("# Title\n\nfirst paragraph\n\nsecond paragraph\n"));
    window.viewer()->renderNow();

    MarkdownView *view = window.viewer();
    QTextBlock target;
    for (QTextBlock block = view->document()->firstBlock(); block.isValid(); block = block.next()) {
        if (block.text() == QLatin1String("second paragraph"))
            target = block;
    }
    QVERIFY(target.isValid());

    const QPoint at = view->cursorRect(QTextCursor(target)).center();
    QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier, at);

    QCOMPARE(window.editor()->textCursor().block().blockNumber(), 4);
    QCOMPARE(window.editor()->textCursor().block().text(), QStringLiteral("second paragraph"));
    // The jump does not steal the focus: the preview can still be used to select
    // and copy text.
    QVERIFY(!window.editor()->hasFocus());
}

void SidebarTest::theSidebarSeparatorIsAHairline()
{
    MainWindow window;
    window.resize(900, 500);
    window.show();
    QApplication::processEvents();
    QApplication::processEvents();

    auto *dock = window.findChild<QDockWidget *>(QStringLiteral("sidebar"));
    QVERIFY(dock);
    QVERIFY(dock->isVisible());

    // The dock separator is the gap the window leaves between the sidebar and
    // the central widget; the stock one is a thick bevel that reads as a wall
    // next to the flat panes.
    const int gap = window.centralWidget()->geometry().x() - dock->geometry().right() - 1;
    QCOMPARE(gap, 1);
}

void SidebarTest::theSidebarTitleBarHasUsableButtons()
{
    MainWindow window;
    window.resize(900, 500);
    window.show();
    QApplication::processEvents();

    auto *dock = window.findChild<QDockWidget *>(QStringLiteral("sidebar"));
    QVERIFY(dock);
    QWidget *titleBar = dock->titleBarWidget();
    QVERIFY(titleBar);

    auto *close = titleBar->findChild<QAbstractButton *>(QStringLiteral("sidebarCloseButton"));
    auto *floatButton = titleBar->findChild<QAbstractButton *>(QStringLiteral("sidebarFloatButton"));
    QVERIFY(close);
    QVERIFY(floatButton);

    // Big enough to hit: the stock ones are about 18x14.
    QVERIFY2(close->width() >= 22 && close->height() >= 22, qPrintable(QStringLiteral("close button is %1x%2").arg(close->width()).arg(close->height())));
    QVERIFY2(floatButton->width() >= 22 && floatButton->height() >= 22,
             qPrintable(QStringLiteral("float button is %1x%2").arg(floatButton->width()).arg(floatButton->height())));

    QTest::mouseClick(close, Qt::LeftButton);
    QVERIFY(!dock->isVisible());
    dock->toggleViewAction()->trigger();
    QVERIFY(dock->isVisible());
}

QTEST_MAIN(SidebarTest)
#include "test_sidebar.moc"
