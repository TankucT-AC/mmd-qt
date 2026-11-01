// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/ScrollSync.h"

#include <QAbstractScrollArea>
#include <QScrollBar>

ScrollSync::ScrollSync(QAbstractScrollArea *first, QAbstractScrollArea *second, QObject *parent)
    : QObject(parent)
    , m_first(first)
    , m_second(second)
{
    connect(m_first->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        syncTo(m_first->verticalScrollBar(), m_second->verticalScrollBar());
    });
    connect(m_second->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
        syncTo(m_second->verticalScrollBar(), m_first->verticalScrollBar());
    });
}

void ScrollSync::syncTo(QScrollBar *from, QScrollBar *to)
{
    if (m_guard)
        return;

    m_guard = true;
    applyFraction(from, to);
    m_guard = false;
}

void ScrollSync::applyFraction(const QScrollBar *from, QScrollBar *to)
{
    const int fromMaximum = from->maximum();
    if (fromMaximum <= 0)
        return;

    const int target = qRound(double(to->maximum()) * from->value() / fromMaximum);
    if (to->value() != target)
        to->setValue(target);
}