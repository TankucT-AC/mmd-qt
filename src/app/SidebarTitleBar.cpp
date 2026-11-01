// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/SidebarTitleBar.h"

#include <QDockWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QToolButton>

namespace
{

constexpr int kButtonSide = 26;
constexpr int kIconSide = 18;

enum class Glyph {
    Float,
    Close,
};

// Drawn rather than borrowed from the style: the glyphs have to be crisp at a
// size the stock icons are not made for, and follow the theme.
QIcon glyphIcon(Glyph glyph, const QColor &color)
{
    QPixmap pixmap(kIconSide, kIconSide);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(color, 1.4));
    painter.setBrush(Qt::NoBrush);

    if (glyph == Glyph::Close) {
        painter.drawLine(QPointF(4.5, 4.5), QPointF(13.5, 13.5));
        painter.drawLine(QPointF(13.5, 4.5), QPointF(4.5, 13.5));
    } else {
        // Two overlapping sheets: the docked pane, and the one that leaves.
        painter.drawRoundedRect(QRectF(3.5, 3.5, 9.0, 9.0), 1.5, 1.5);
        painter.drawRoundedRect(QRectF(7.5, 7.5, 7.0, 7.0), 1.5, 1.5);
    }

    painter.end();
    return QIcon(pixmap);
}

QToolButton *makeButton(const QString &objectName, const QString &tooltip, QWidget *parent)
{
    auto *button = new QToolButton(parent);
    button->setObjectName(objectName);
    button->setAutoRaise(true);
    button->setIconSize(QSize(kIconSide, kIconSide));
    button->setFixedSize(kButtonSide, kButtonSide);
    button->setToolTip(tooltip);
    return button;
}

} // namespace

SidebarTitleBar::SidebarTitleBar(QDockWidget *dock, const QString &title, QWidget *parent)
    : QWidget(parent)
{
    auto *label = new QLabel(title, this);
    label->setContentsMargins(6, 0, 0, 0);

    m_float = makeButton(QStringLiteral("sidebarFloatButton"), tr("Float the sidebar"), this);
    m_close = makeButton(QStringLiteral("sidebarCloseButton"), tr("Close the sidebar"), this);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(m_float);
    layout->addWidget(m_close);

    connect(m_close, &QToolButton::clicked, dock, &QWidget::hide);
    connect(m_float, &QToolButton::clicked, dock, [dock] {
        dock->setFloating(!dock->isFloating());
    });
}

void SidebarTitleBar::setTheme(const ThemeDefinition &theme)
{
    const QColor ink = theme.palette.color(QPalette::WindowText);
    m_float->setIcon(glyphIcon(Glyph::Float, ink));
    m_close->setIcon(glyphIcon(Glyph::Close, ink));
}
