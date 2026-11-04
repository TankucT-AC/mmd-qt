// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Renders the app icon PNGs from resources/logo.svg.
 *
 * The SVG is the source of truth: resources/logo-*.png are derived from it and
 * are what the window icon and the installed desktop icon are built from. Small
 * sizes are supersampled and scaled down, because Qt's SVG renderer antialiases
 * coarsely when a 256 px drawing is squeezed into 16 px.
 *
 * Not part of the build; run it by hand after editing the SVG:
 *
 *   g++ -std=c++20 -fPIC $(pkg-config --cflags Qt6Svg) tools/render-icons.cpp \
 *       -o /tmp/render-icons $(pkg-config --libs Qt6Svg)
 *   QT_QPA_PLATFORM=offscreen /tmp/render-icons
 *
 * The sizes it writes must match resources/resources.qrc and the list in
 * src/main.cpp.
 */

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>

#include <cstdio>

namespace
{

// Relative to the repository root, which is where this should be run from.
constexpr auto kSource = "resources/logo.svg";
constexpr auto kTarget = "resources/logo-%1.png";

// The sizes the app and the desktop integration ship.
constexpr int kSizes[] = { 16, 24, 32, 48, 64, 128, 256 };

} // namespace

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);

    QSvgRenderer renderer(QString::fromLatin1(kSource));
    if (!renderer.isValid()) {
        std::printf("cannot load %s (run me from the repository root)\n", kSource);
        return 1;
    }

    for (int size : kSizes) {
        // Supersample the small sizes: the renderer's own antialiasing is coarse
        // when the drawing is squeezed.
        const int factor = size <= 64 ? 8 : 2;
        QImage big(size * factor, size * factor, QImage::Format_ARGB32_Premultiplied);
        big.fill(Qt::transparent);
        {
            QPainter painter(&big);
            painter.setRenderHint(QPainter::Antialiasing, true);
            renderer.render(&painter, QRectF(0, 0, big.width(), big.height()));
        }

        const QImage small = big.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        const QString target = QString::fromLatin1(kTarget).arg(size);
        if (!small.save(target, "PNG")) {
            std::printf("cannot write %s\n", qPrintable(target));
            return 1;
        }
        std::printf("wrote %s\n", qPrintable(target));
    }
    return 0;
}
