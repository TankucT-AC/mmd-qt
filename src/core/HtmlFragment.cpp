// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/HtmlFragment.h"

namespace
{

bool isNameChar(QChar c)
{
    return c.isLetterOrNumber() || c == QLatin1Char('-') || c == QLatin1Char('_') || c == QLatin1Char(':');
}

bool isSpace(QChar c)
{
    return c.isSpace();
}

// Advances past whitespace, returning the first non-space index (or size()).
int skipSpaces(const QString &html, int index)
{
    while (index < html.size() && isSpace(html.at(index)))
        ++index;
    return index;
}

QString readName(const QString &html, int &index)
{
    const int start = index;
    while (index < html.size() && isNameChar(html.at(index)))
        ++index;
    return html.mid(start, index - start).toLower();
}

// Reads one attribute starting at index (already past the tag name). Returns
// false when there is nothing left to read before '>' or '/>'.
bool readAttribute(const QString &html, int &index, std::pair<QString, QString> &out)
{
    index = skipSpaces(html, index);
    if (index >= html.size())
        return false;
    const QChar c = html.at(index);
    if (c == QLatin1Char('>') || c == QLatin1Char('/'))
        return false;

    const QString name = readName(html, index);
    if (name.isEmpty()) {
        ++index; // skip a stray character so we always make progress
        return false;
    }

    index = skipSpaces(html, index);
    QString value;
    if (index < html.size() && html.at(index) == QLatin1Char('=')) {
        ++index;
        index = skipSpaces(html, index);
        if (index < html.size() && (html.at(index) == QLatin1Char('"') || html.at(index) == QLatin1Char('\''))) {
            const QChar quote = html.at(index);
            ++index;
            const int start = index;
            while (index < html.size() && html.at(index) != quote)
                ++index;
            value = html.mid(start, index - start);
            if (index < html.size())
                ++index; // consume closing quote
        } else {
            const int start = index;
            while (index < html.size() && !isSpace(html.at(index)) && html.at(index) != QLatin1Char('>'))
                ++index;
            value = html.mid(start, index - start);
        }
    }

    out = {name, value};
    return true;
}

} // namespace

bool isHtmlVoidElement(const QString &name)
{
    static const QVector<QString> kVoid = {QStringLiteral("br"),
                                           QStringLiteral("img"),
                                           QStringLiteral("hr"),
                                           QStringLiteral("input"),
                                           QStringLiteral("meta"),
                                           QStringLiteral("link"),
                                           QStringLiteral("area"),
                                           QStringLiteral("base"),
                                           QStringLiteral("col"),
                                           QStringLiteral("embed"),
                                           QStringLiteral("source"),
                                           QStringLiteral("track"),
                                           QStringLiteral("wbr")};
    return kVoid.contains(name);
}

QString htmlAttribute(const HtmlToken &token, const QString &name)
{
    const QString key = name.toLower();
    for (const auto &attribute : token.attributes) {
        if (attribute.first == key)
            return attribute.second;
    }
    return QString();
}

QVector<HtmlToken> parseHtmlFragment(const QString &html)
{
    QVector<HtmlToken> tokens;
    int index = 0;

    while (index < html.size()) {
        const int lt = html.indexOf(QLatin1Char('<'), index);

        if (lt < 0) {
            tokens.append({HtmlToken::Text, QString(), {}, html.mid(index), false});
            break;
        }

        if (lt > index)
            tokens.append({HtmlToken::Text, QString(), {}, html.mid(index, lt - index), false});

        // Comment.
        if (html.mid(lt, 4) == QLatin1String("<!--")) {
            const int end = html.indexOf(QLatin1String("-->"), lt + 4);
            HtmlToken token;
            token.kind = HtmlToken::Comment;
            token.text = end < 0 ? html.mid(lt + 4) : html.mid(lt + 4, end - lt - 4);
            tokens.append(token);
            index = end < 0 ? html.size() : end + 3;
            continue;
        }

        // Declaration (<!DOCTYPE ...> or <![CDATA[ ... ]]).
        if (html.mid(lt, 2) == QLatin1String("<!")) {
            const int end = html.indexOf(QLatin1Char('>'), lt + 2);
            HtmlToken token;
            token.kind = HtmlToken::Declaration;
            token.text = end < 0 ? html.mid(lt + 2) : html.mid(lt + 2, end - lt - 2);
            tokens.append(token);
            index = end < 0 ? html.size() : end + 1;
            continue;
        }

        // Closing tag.
        if (html.mid(lt, 2) == QLatin1String("</")) {
            int cursor = lt + 2;
            HtmlToken token;
            token.kind = HtmlToken::EndTag;
            token.name = readName(html, cursor);
            const int end = html.indexOf(QLatin1Char('>'), cursor);
            index = end < 0 ? html.size() : end + 1;
            if (token.name.isEmpty()) {
                // "</" with no name: treat as text.
                tokens.append({HtmlToken::Text, QString(), {}, html.mid(lt, index - lt), false});
            } else {
                tokens.append(token);
            }
            continue;
        }

        // Opening tag (or a stray '<' that is not a tag).
        int cursor = lt + 1;
        const QString name = readName(html, cursor);
        if (name.isEmpty()) {
            tokens.append({HtmlToken::Text, QString(), {}, QStringLiteral("<"), false});
            index = lt + 1;
            continue;
        }

        HtmlToken token;
        token.kind = HtmlToken::StartTag;
        token.name = name;
        while (true) {
            std::pair<QString, QString> attribute;
            const int before = cursor;
            if (!readAttribute(html, cursor, attribute)) {
                if (cursor == before)
                    break;
                continue;
            }
            token.attributes.append(attribute);
        }

        cursor = skipSpaces(html, cursor);
        if (cursor < html.size() && html.at(cursor) == QLatin1Char('/')) {
            token.selfClosing = true;
            ++cursor;
        }
        const int end = html.indexOf(QLatin1Char('>'), cursor);
        index = end < 0 ? html.size() : end + 1;

        if (token.selfClosing || isHtmlVoidElement(name))
            token.kind = HtmlToken::SelfClosingTag;
        tokens.append(token);
    }

    return tokens;
}