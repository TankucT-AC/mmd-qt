// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class QAbstractScrollArea;
class QScrollBar;

/*
 * Keeps the vertical scroll positions of two scroll areas in step, so the
 * source pane and the preview pane show the same region of the document.
 *
 * Synchronisation is proportional: the fraction scrolled in one pane is
 * applied to the other. This is deliberately not a line-by-line mapping —
 * rendered markdown collapses source lines, and a proportional link stays
 * stable while the document is being edited.
 *
 * A re-entrancy guard stops the two views from bouncing the value back and
 * forth forever.
 */
class ScrollSync : public QObject
{
    Q_OBJECT

public:
    // Neither area is owned; both must outlive the synchronizer.
    ScrollSync(QAbstractScrollArea *first, QAbstractScrollArea *second, QObject *parent = nullptr);

private:
    void syncTo(QScrollBar *from, QScrollBar *to);
    static void applyFraction(const QScrollBar *from, QScrollBar *to);

    QAbstractScrollArea *m_first;
    QAbstractScrollArea *m_second;
    bool m_guard = false;
};