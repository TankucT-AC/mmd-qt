// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/MarkdownBuilder.h"
#include "core/Outline.h"
#include "core/Theme.h"

#include <QTextBlock>
#include <QTextDocument>

class OutlineTest : public QObject
{
    Q_OBJECT

private slots:
    void extractsHeadingsInDocumentOrder();
    void keepsTheLevelsAndSlugs();
    void ignoresHeadingsInsideFences();
    void reportsBlocksThatExist();
    void sourceLinesSkipFencedCode();

private:
    static std::unique_ptr<QTextDocument> build(const QString &markdown);
};

std::unique_ptr<QTextDocument> OutlineTest::build(const QString &markdown)
{
    RenderStyle style;
    style.theme = ThemeDefinition::light();
    auto document = std::make_unique<QTextDocument>();
    MarkdownBuilder::build(document.get(), markdown, style);
    return document;
}

void OutlineTest::extractsHeadingsInDocumentOrder()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("# A\n\ntext\n\n## B\n\nmore\n\n### C\n\n# D\n")));

    const QVector<OutlineEntry> entries = Outline::fromDocument(*document);
    QCOMPARE(entries.size(), 4);
    QCOMPARE(entries.at(0).title, QStringLiteral("A"));
    QCOMPARE(entries.at(1).title, QStringLiteral("B"));
    QCOMPARE(entries.at(2).title, QStringLiteral("C"));
    QCOMPARE(entries.at(3).title, QStringLiteral("D"));
}

void OutlineTest::keepsTheLevelsAndSlugs()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("# A\n\n## B\n\n### C\n\n# D\n")));

    const QVector<OutlineEntry> entries = Outline::fromDocument(*document);
    QCOMPARE(entries.at(0).level, 1);
    QCOMPARE(entries.at(1).level, 2);
    QCOMPARE(entries.at(2).level, 3);
    QCOMPARE(entries.at(3).level, 1);
    QCOMPARE(entries.at(0).slug, QStringLiteral("a"));
    // The slug is what in-document links resolve against.
    QCOMPARE(entries.at(1).slug, MarkdownBuilder::headingSlug(QStringLiteral("B")));
}

void OutlineTest::ignoresHeadingsInsideFences()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("# Real\n\n```\n# not a heading\n```\n\n## Also real\n")));

    const QVector<OutlineEntry> entries = Outline::fromDocument(*document);
    QCOMPARE(entries.size(), 2);
    QCOMPARE(entries.at(0).title, QStringLiteral("Real"));
    QCOMPARE(entries.at(1).title, QStringLiteral("Also real"));
}

void OutlineTest::reportsBlocksThatExist()
{
    std::unique_ptr<QTextDocument> document(build(QStringLiteral("intro\n\n# One\n\nbody\n\n## Two\n")));

    const QVector<OutlineEntry> entries = Outline::fromDocument(*document);
    QCOMPARE(entries.size(), 2);
    for (const OutlineEntry &entry : entries) {
        const QTextBlock block = document->findBlockByNumber(entry.blockNumber);
        QVERIFY(block.isValid());
        QCOMPARE(block.text(), entry.title);
    }
}

void OutlineTest::sourceLinesSkipFencedCode()
{
    const QString markdown = QStringLiteral(
        "# One\n"
        "\n"
        "```\n"
        "# not a heading\n"
        "```\n"
        "\n"
        "## Two\n"
        "#hashtag is not a heading\n"
        "   # Three\n");

    const QVector<int> lines = Outline::headingSourceLines(markdown);
    QCOMPARE(lines.size(), 3);
    QCOMPARE(lines.at(0), 0); // "# One"
    QCOMPARE(lines.at(1), 6); // "## Two", past the fence and the hashtag
    QCOMPARE(lines.at(2), 8); // "   # Three", indented up to three spaces
}

QTEST_MAIN(OutlineTest)
#include "test_outline.moc"
