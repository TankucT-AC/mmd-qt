// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/FindBar.h"

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QStyle>
#include <QToolButton>

FindBar::FindBar(QWidget *parent)
    : QWidget(parent)
    , m_input(new QLineEdit(this))
    , m_status(new QLabel(this))
    , m_previous(new QToolButton(this))
    , m_next(new QToolButton(this))
    , m_case(new QToolButton(this))
{
    m_input->setPlaceholderText(tr("Find"));
    m_input->setClearButtonEnabled(true);
    m_input->installEventFilter(this); // to see Escape and Shift+Enter

    // A fixed slot for the counter, so the row does not jump as it changes.
    m_status->setMinimumWidth(72);

    m_previous->setIcon(style()->standardIcon(QStyle::SP_ArrowUp));
    m_previous->setToolTip(tr("Previous match (Shift+F3)"));
    m_next->setIcon(style()->standardIcon(QStyle::SP_ArrowDown));
    m_next->setToolTip(tr("Next match (F3)"));

    m_case->setText(tr("Aa"));
    m_case->setCheckable(true);
    m_case->setToolTip(tr("Match case"));

    auto *close = new QToolButton(this);
    close->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    close->setToolTip(tr("Close (Esc)"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);
    layout->addWidget(m_input, 1);
    layout->addWidget(m_status);
    layout->addWidget(m_case);
    layout->addWidget(m_previous);
    layout->addWidget(m_next);
    layout->addWidget(close);

    connect(m_input, &QLineEdit::textChanged, this, &FindBar::emitQueryChanged);
    connect(m_case, &QToolButton::toggled, this, &FindBar::emitQueryChanged);
    connect(m_next, &QToolButton::clicked, this, &FindBar::findNextRequested);
    connect(m_previous, &QToolButton::clicked, this, &FindBar::findPreviousRequested);
    connect(close, &QToolButton::clicked, this, &FindBar::closed);
}

void FindBar::emitQueryChanged()
{
    emit queryChanged(m_input->text(), m_case->isChecked());
}

QString FindBar::query() const
{
    return m_input->text();
}

bool FindBar::caseSensitive() const
{
    return m_case->isChecked();
}

void FindBar::clearQuery()
{
    m_input->clear(); // the change signal does the rest
}

void FindBar::setQuery(const QString &query)
{
    m_input->setText(query); // the change signal runs the search
}

void FindBar::setStatus(const QString &status)
{
    m_status->setText(status);
}

QString FindBar::status() const
{
    return m_status->text();
}

void FindBar::clearStatus()
{
    m_status->clear();
}

void FindBar::focusInput()
{
    m_input->setFocus(Qt::ShortcutFocusReason);
    m_input->selectAll();
}

bool FindBar::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_input && event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent *>(event);
        switch (key->key()) {
        case Qt::Key_Escape:
            emit closed();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_F3:
            if (key->modifiers() & Qt::ShiftModifier)
                emit findPreviousRequested();
            else
                emit findNextRequested();
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void FindBar::keyPressEvent(QKeyEvent *event)
{
    // Reached when a button has the focus: Escape still closes the bar, and the
    // buttons stay reachable by keyboard.
    if (event->key() == Qt::Key_Escape) {
        emit closed();
        return;
    }
    QWidget::keyPressEvent(event);
}
