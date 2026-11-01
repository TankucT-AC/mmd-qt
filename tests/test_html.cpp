// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include "core/HtmlFragment.h"

class HtmlFragmentTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesStartAndEndTags();
    void parsesAttributesInAllQuotingStyles();
    void treatsVoidElementsAsSelfClosing();
    void handlesCommentsAndDeclarations();
    void tolerantOnMalformedInput();
};

void HtmlFragmentTest::parsesStartAndEndTags()
{
    const QVector<HtmlToken> tokens = parseHtmlFragment(QStringLiteral("<div><strong>hi</strong></div>"));
    QCOMPARE(tokens.size(), 5);
    QCOMPARE(tokens[0].kind, HtmlToken::StartTag);
    QCOMPARE(tokens[0].name, QStringLiteral("div"));
    QCOMPARE(tokens[1].kind, HtmlToken::StartTag);
    QCOMPARE(tokens[1].name, QStringLiteral("strong"));
    QCOMPARE(tokens[2].kind, HtmlToken::Text);
    QCOMPARE(tokens[2].text, QStringLiteral("hi"));
    QCOMPARE(tokens[3].kind, HtmlToken::EndTag);
    QCOMPARE(tokens[3].name, QStringLiteral("strong"));
    QCOMPARE(tokens[4].kind, HtmlToken::EndTag);
    QCOMPARE(tokens[4].name, QStringLiteral("div"));
}

void HtmlFragmentTest::parsesAttributesInAllQuotingStyles()
{
    const QVector<HtmlToken> tokens = parseHtmlFragment(QStringLiteral("<img src=\"a.png\" alt='x' data-y=z checked/>"));
    QCOMPARE(tokens.size(), 1);
    const HtmlToken &tag = tokens.first();
    QCOMPARE(tag.kind, HtmlToken::SelfClosingTag);
    QCOMPARE(htmlAttribute(tag, QStringLiteral("src")), QStringLiteral("a.png"));
    QCOMPARE(htmlAttribute(tag, QStringLiteral("ALT")), QStringLiteral("x"));
    QCOMPARE(htmlAttribute(tag, QStringLiteral("data-y")), QStringLiteral("z"));
    QCOMPARE(htmlAttribute(tag, QStringLiteral("checked")), QString());
}

void HtmlFragmentTest::treatsVoidElementsAsSelfClosing()
{
    const QVector<HtmlToken> tokens = parseHtmlFragment(QStringLiteral("<br>done"));
    QCOMPARE(tokens.size(), 2);
    QCOMPARE(tokens[0].kind, HtmlToken::SelfClosingTag);
    QCOMPARE(tokens[0].name, QStringLiteral("br"));
    QCOMPARE(tokens[1].kind, HtmlToken::Text);
    QCOMPARE(tokens[1].text, QStringLiteral("done"));
}

void HtmlFragmentTest::handlesCommentsAndDeclarations()
{
    const QVector<HtmlToken> tokens = parseHtmlFragment(QStringLiteral("<div><!-- note --></div><!DOCTYPE html>"));
    QCOMPARE(tokens.size(), 4);
    QCOMPARE(tokens[1].kind, HtmlToken::Comment);
    QCOMPARE(tokens[1].text.trimmed(), QStringLiteral("note"));
    QCOMPARE(tokens[3].kind, HtmlToken::Declaration);
}

void HtmlFragmentTest::tolerantOnMalformedInput()
{
    // An unterminated tag and a stray '<' must not crash and must not lose text.
    const QVector<HtmlToken> tokens = parseHtmlFragment(QStringLiteral("a < b and <unclosed attr=\"v\""));
    QVERIFY(!tokens.isEmpty());
    QString text;
    for (const HtmlToken &token : tokens)
        text += token.text;
    QVERIFY(text.contains(QStringLiteral("a ")));
    QVERIFY(text.contains(QStringLiteral(" b and ")));
}

QTEST_MAIN(HtmlFragmentTest)
#include "test_html.moc"