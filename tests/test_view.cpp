// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "app/ScrollSync.h"
#include "core/Theme.h"
#include "viewer/MarkdownView.h"

#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSet>
#include <QTextBlock>
#include <QTextBrowser>

class ViewTest : public QObject
{
    Q_OBJECT

private slots:
    void rendersMarkdownIntoDocument();
    void previewHighlightsFencedCode();
    void themeSwitchRewritesDocument();
    void burstOfChangesCoalescesIntoOneRender();
    void renderIsImmediateWhenIntervalIsZero();
    void scrollIsPreservedAcrossRerender();
    void baseDirResolvesImages();
    void scrollSyncIsProportionalAndGuarded();
    void scrollSyncDoesNotLoop();
};

void ViewTest::rendersMarkdownIntoDocument()
{
    MarkdownView view;
    view.setRenderInterval(0);
    view.setMarkdown(QStringLiteral("# Title\n\ntext\n"));

    const QString text = view.document()->toPlainText();
    QVERIFY(text.contains(QStringLiteral("Title")));
    QVERIFY(text.contains(QStringLiteral("text")));
}

void ViewTest::previewHighlightsFencedCode()
{
    MarkdownView view;
    view.setRenderInterval(0);
    view.setMarkdown(QStringLiteral("```cpp\n// note\nint x = 0;\n```\n"));

    QSet<QRgb> colors;
    for (QTextBlock block = view.document()->firstBlock(); block.isValid(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it)
            colors.insert(it.fragment().charFormat().foreground().color().rgb());
    }
    // The comment and the keyword got colors distinct from plain code text.
    QVERIFY(colors.size() >= 3);
}

void ViewTest::themeSwitchRewritesDocument()
{
    MarkdownView view;
    view.setRenderInterval(0);
    view.setMarkdown(QStringLiteral("# Title\n"));

    RenderStyle style;
    style.theme = ThemeDefinition::light();
    view.setRenderStyle(style);
    QVERIFY(view.document()->firstBlock().charFormat().foreground().color() == ThemeDefinition::light().markdown.heading);

    style.theme = ThemeDefinition::dark();
    view.setRenderStyle(style);
    QCOMPARE(view.document()->firstBlock().charFormat().foreground().color(), ThemeDefinition::dark().markdown.heading);
}

void ViewTest::burstOfChangesCoalescesIntoOneRender()
{
    MarkdownView view;
    view.setRenderInterval(50);

    QSignalSpy spy(&view, &MarkdownView::rendered);
    for (int i = 0; i < 5; ++i)
        view.setMarkdown(QStringLiteral("# Title %1\n").arg(i));

    QCOMPARE(spy.count(), 0); // nothing yet: the render is coalesced
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 1000);
    QVERIFY(view.document()->toPlainText().contains(QStringLiteral("Title 4")));
}

void ViewTest::renderIsImmediateWhenIntervalIsZero()
{
    MarkdownView view;
    view.setRenderInterval(0);
    QSignalSpy spy(&view, &MarkdownView::rendered);
    view.setMarkdown(QStringLiteral("# Now\n"));
    QCOMPARE(spy.count(), 1);
}

void ViewTest::scrollIsPreservedAcrossRerender()
{
    MarkdownView view;
    view.resize(400, 300);
    view.show();
    view.setRenderInterval(0);

    QString body;
    for (int i = 0; i < 200; ++i)
        body += QStringLiteral("line %1\n\n").arg(i);
    view.setMarkdown(body);

    view.verticalScrollBar()->setValue(view.verticalScrollBar()->maximum() / 2);
    const int before = view.verticalScrollBar()->value();
    const QString topBefore = view.cursorForPosition(QPoint(0, 0)).block().text();
    QVERIFY(before > 0);

    // Append more text: the block at the top must stay at the top.
    view.setMarkdown(body + QStringLiteral("added at the end\n"));
    const QString topAfter = view.cursorForPosition(QPoint(0, 0)).block().text();
    QCOMPARE(topAfter, topBefore);
}

void ViewTest::baseDirResolvesImages()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile png(dir.filePath(QStringLiteral("pic.png")));
    QVERIFY(png.open(QIODevice::WriteOnly));
    png.write(QByteArray::fromBase64("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAEhQGAhKmMIQAAAABJRU5ErkJggg=="));
    png.close();

    MarkdownView view;
    view.setRenderInterval(0);
    view.setBaseDir(dir.path());
    view.setMarkdown(QStringLiteral("![alt](pic.png)\n"));

    bool found = false;
    for (QTextBlock block = view.document()->firstBlock(); block.isValid(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            if (it.fragment().charFormat().isImageFormat())
                found = true;
        }
    }
    QVERIFY(found);
}

void ViewTest::scrollSyncIsProportionalAndGuarded()
{
    QTextBrowser first;
    QTextBrowser second;

    // Explicit ranges keep the test independent of layout timing.
    first.verticalScrollBar()->setRange(0, 1000);
    second.verticalScrollBar()->setRange(0, 500);

    ScrollSync sync(&first, &second);

    first.verticalScrollBar()->setValue(250); // quarter of the way down
    QCOMPARE(second.verticalScrollBar()->value(), 125); // same fraction of 500

    second.verticalScrollBar()->setValue(500); // full
    QCOMPARE(first.verticalScrollBar()->value(), 1000);
}

void ViewTest::scrollSyncDoesNotLoop()
{
    QTextBrowser first;
    QTextBrowser second;
    first.resize(300, 200);
    second.resize(300, 200);
    first.verticalScrollBar()->setRange(0, 100);
    second.verticalScrollBar()->setRange(0, 100);

    ScrollSync sync(&first, &second);
    QSignalSpy spy(first.verticalScrollBar(), &QScrollBar::valueChanged);
    second.verticalScrollBar()->setValue(50);
    // One forward update only; the guard must prevent bouncing back.
    QCOMPARE(first.verticalScrollBar()->value(), 50);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(ViewTest)
#include "test_view.moc"