// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/Outline.h"

#include "core/MarkdownBuilder.h"

#include <QStringView>
#include <QTextBlock>
#include <QTextDocument>

#include <algorithm>

namespace
{

// A line that opens with "# Heading": up to three leading spaces, one to six
// hashes, and a space or the end of the line after them, so "#hashtag" stays
// prose. Hand-rolled rather than a regular expression, because this runs once
// per line on documents of any size.
bool isAtxHeading(QStringView line)
{
    qsizetype index = 0;
    while (index < line.size() && index < 3 && line.at(index) == QLatin1Char(' '))
        ++index;

    qsizetype hashes = 0;
    while (index + hashes < line.size() && hashes < 7 && line.at(index + hashes) == QLatin1Char('#'))
        ++hashes;
    if (hashes < 1 || hashes > 6)
        return false;

    const qsizetype after = index + hashes;
    return after >= line.size() || line.at(after) == QLatin1Char(' ') || line.at(after) == QLatin1Char('\t');
}

// The fence marker of a line ("```" or "~~~", at least three of them), or an
// empty view for every other line.
QStringView fenceMarker(QStringView line)
{
    qsizetype index = 0;
    while (index < line.size() && index < 3 && line.at(index) == QLatin1Char(' '))
        ++index;
    if (index >= line.size())
        return {};

    const QChar character = line.at(index);
    if (character != QLatin1Char('`') && character != QLatin1Char('~'))
        return {};

    qsizetype end = index;
    while (end < line.size() && line.at(end) == character)
        ++end;
    if (end - index < 3)
        return {};

    return line.mid(index, end - index);
}

} // namespace

QVector<OutlineEntry> Outline::fromDocument(const QTextDocument &document)
{
    QVector<OutlineEntry> entries;

    for (QTextBlock block = document.firstBlock(); block.isValid(); block = block.next()) {
        const QTextBlockFormat format = block.blockFormat();
        if (format.intProperty(MarkdownBuilder::kindKey) != int(MarkdownBuilder::BlockKind::Heading))
            continue;

        OutlineEntry entry;
        entry.level = std::clamp(format.intProperty(MarkdownBuilder::headingLevelKey), 1, 6);
        entry.title = block.text();
        entry.slug = MarkdownBuilder::headingSlug(entry.title);
        entry.blockNumber = block.blockNumber();
        entries.append(entry);
    }

    return entries;
}

QVector<int> Outline::headingSourceLines(const QString &markdown)
{
    QVector<int> lines;
    const QStringView text(markdown);

    bool inFence = false;
    QChar fenceCharacter;
    qsizetype fenceLength = 0;
    int lineNumber = 0;

    qsizetype start = 0;
    while (start <= text.size()) {
        const qsizetype newline = text.indexOf(QLatin1Char('\n'), start);
        const QStringView line = text.mid(start, newline < 0 ? -1 : newline - start);

        const QStringView marker = fenceMarker(line);
        if (inFence) {
            // Only the character that opened the block closes it, and only when
            // the closing fence is at least as long.
            if (!marker.isEmpty() && marker.at(0) == fenceCharacter && marker.size() >= fenceLength)
                inFence = false;
        } else if (!marker.isEmpty()) {
            inFence = true;
            fenceCharacter = marker.at(0);
            fenceLength = marker.size();
        } else if (isAtxHeading(line)) {
            lines.append(lineNumber);
        }

        if (newline < 0)
            break;
        start = newline + 1;
        ++lineNumber;
    }

    return lines;
}
