// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/FindState.h"

void FindState::setQuery(const QString &query, bool caseSensitive)
{
    m_query = query;
    m_caseSensitive = caseSensitive;
}

void FindState::refresh(const QString &text)
{
    const int previous = m_current;
    m_matches = findMatches(text);

    if (m_matches.isEmpty()) {
        m_current = -1;
    } else if (previous >= 0 && previous < m_matches.size()) {
        m_current = previous;
    } else {
        m_current = 0;
    }
}

FindState::Match FindState::current() const
{
    if (m_current < 0 || m_current >= m_matches.size())
        return Match();
    return m_matches.at(m_current);
}

void FindState::selectNext(int fromPosition)
{
    if (m_matches.isEmpty()) {
        m_current = -1;
        return;
    }
    const int index = firstIndexAtOrAfter(m_matches, fromPosition);
    m_current = index < m_matches.size() ? index : 0; // wrap to the top
}

void FindState::selectPrevious(int before)
{
    if (m_matches.isEmpty()) {
        m_current = -1;
        return;
    }
    const int index = lastIndexBefore(m_matches, before);
    m_current = index >= 0 ? index : int(m_matches.size()) - 1; // wrap to the bottom
}

void FindState::clear()
{
    m_query.clear();
    m_matches.clear();
    m_current = -1;
}

QVector<FindState::Match> FindState::findMatches(const QString &text) const
{
    QVector<Match> found;
    if (m_query.isEmpty())
        return found;

    const int needle = int(m_query.size());
    const Qt::CaseSensitivity sensitivity = m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

    int from = 0;
    while (from <= text.size()) {
        const qsizetype at = text.indexOf(m_query, from, sensitivity);
        if (at < 0)
            break;
        found.append({int(at), needle});
        from = int(at) + needle; // non-overlapping: "aa" in "aaaa" is two matches
    }
    return found;
}

int FindState::firstIndexAtOrAfter(const QVector<Match> &matches, int position)
{
    for (int i = 0; i < matches.size(); ++i) {
        if (matches.at(i).position >= position)
            return i;
    }
    return int(matches.size());
}

int FindState::lastIndexBefore(const QVector<Match> &matches, int position)
{
    for (int i = int(matches.size()) - 1; i >= 0; --i) {
        if (matches.at(i).position < position)
            return i;
    }
    return -1;
}
