// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QColor>
#include <QObject>
#include <QTextObjectInterface>

/*
 * Draws inline "chips" inside the preview — small rounded labels used for the
 * alt text of remote images (e.g. shields.io badges) and for <kbd> keys.
 *
 * Remote images are not fetched: a chip keeps the markdown readable offline,
 * instantly and without touching the network.
 */
class ChipObjectHandler : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit ChipObjectHandler(QObject *parent = nullptr);

    void setColors(const QColor &background, const QColor &border, const QColor &text);

    QSizeF intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format) override;
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format) override;

private:
    QColor m_background;
    QColor m_border;
    QColor m_text;
};