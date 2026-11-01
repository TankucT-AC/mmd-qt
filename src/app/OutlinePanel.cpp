// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/OutlinePanel.h"

#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{

// Whether two outlines describe the same document structure. Block numbers are
// deliberately ignored: they shift on every edit above a heading, and rebuilding
// the tree for that would throw away the selection for nothing.
bool sameStructure(const QVector<OutlineEntry> &lhs, const QVector<OutlineEntry> &rhs)
{
    if (lhs.size() != rhs.size())
        return false;
    for (int i = 0; i < lhs.size(); ++i) {
        if (lhs.at(i).level != rhs.at(i).level || lhs.at(i).title != rhs.at(i).title)
            return false;
    }
    return true;
}

} // namespace

OutlinePanel::OutlinePanel(QWidget *parent)
    : QWidget(parent)
    , m_tree(new QTreeWidget(this))
{
    m_tree->setHeaderHidden(true);
    m_tree->setUniformRowHeights(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tree);

    // itemClicked covers the mouse, itemActivated the keyboard (Enter).
    connect(m_tree, &QTreeWidget::itemClicked, this, &OutlinePanel::activate);
    connect(m_tree, &QTreeWidget::itemActivated, this, &OutlinePanel::activate);
}

void OutlinePanel::activate(QTreeWidgetItem *item)
{
    if (!item)
        return;
    const int index = item->data(0, Qt::UserRole).toInt();
    if (index >= 0 && index < m_entries.size())
        emit entryActivated(index);
}

void OutlinePanel::setOutline(const QVector<OutlineEntry> &entries)
{
    // The rows are rebuilt only when the headings themselves change; the fresh
    // copy still replaces the old one, so block numbers stay current.
    const bool structureChanged = !sameStructure(m_entries, entries);
    m_entries = entries;
    if (structureChanged)
        rebuildTree();
}

void OutlinePanel::rebuildTree()
{
    m_items.clear();
    m_tree->clear();

    // Headings nest by level: a deeper heading becomes a child of the closest
    // shallower one before it, which is what a document outline means.
    QVector<QTreeWidgetItem *> openItems;
    QVector<int> openLevels;
    for (int i = 0; i < m_entries.size(); ++i) {
        const OutlineEntry &entry = m_entries.at(i);
        while (!openLevels.isEmpty() && openLevels.last() >= entry.level) {
            openLevels.removeLast();
            openItems.removeLast();
        }

        auto *item = openItems.isEmpty() ? new QTreeWidgetItem(m_tree) : new QTreeWidgetItem(openItems.last());
        item->setText(0, entry.title);
        item->setToolTip(0, entry.title);
        item->setData(0, Qt::UserRole, i);
        openItems.append(item);
        openLevels.append(entry.level);
        m_items.insert(i, item);
    }

    m_tree->expandAll();
}

void OutlinePanel::setCurrentBlock(int blockNumber)
{
    if (blockNumber < 0) {
        m_tree->setCurrentItem(nullptr);
        return;
    }

    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).blockNumber != blockNumber)
            continue;

        QTreeWidgetItem *item = m_items.value(i);
        if (!item || item == m_tree->currentItem())
            return;

        // Open the ancestors, or the row is selected but not on screen.
        for (QTreeWidgetItem *parent = item->parent(); parent; parent = parent->parent())
            parent->setExpanded(true);
        m_tree->setCurrentItem(item);
        m_tree->scrollToItem(item);
        return;
    }

    // The heading scrolled out of the document (or was edited away).
    m_tree->setCurrentItem(nullptr);
}
