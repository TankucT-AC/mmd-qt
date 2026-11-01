// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class QEvent;
class QLabel;
class QLineEdit;
class QToolButton;

/*
 * The find strip that appears above a pane: a query field, the match counter,
 * and the buttons that walk the matches.
 *
 * It is deliberately dumb — it reports what the user asked for and displays the
 * count back. Searching is FindState's job and the wiring is FindController's.
 *
 * Colours come from the application palette, which the theme sets, so the bar
 * follows a theme switch like every other widget.
 */
class FindBar : public QWidget
{
    Q_OBJECT

public:
    explicit FindBar(QWidget *parent = nullptr);

    void focusInput();

    // Sets the query as if the user had typed it; the search runs immediately.
    void setQuery(const QString &query);

    // "3 of 12", "no results", or an empty string to clear the label.
    void setStatus(const QString &status);
    void clearStatus();
    QString status() const;

    QString query() const;
    bool caseSensitive() const;
    void clearQuery();

signals:
    void queryChanged(const QString &query, bool caseSensitive);
    void findNextRequested();
    void findPreviousRequested();
    void closed();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void emitQueryChanged();

    QLineEdit *m_input = nullptr;
    QLabel *m_status = nullptr;
    QToolButton *m_previous = nullptr;
    QToolButton *m_next = nullptr;
    QToolButton *m_case = nullptr;
};
