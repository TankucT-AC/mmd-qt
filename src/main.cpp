// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>

// Multi-size app icon from the bundled resources, so the window/taskbar gets a
// crisp mark at every size instead of one scaled bitmap.
static QIcon appIcon()
{
    QIcon icon;
    for (int size : { 16, 32, 64, 128, 256 })
        icon.addFile(QStringLiteral(":/mmd/logo/logo-%1.png").arg(size));
    return icon;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("mmd"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("mmd-qt"));
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setWindowIcon(appIcon());

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Two-pane Markdown viewer and editor"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Markdown file to open."), QStringLiteral("[file]"));

    QCommandLineOption previewOnly(QStringLiteral("preview"), QStringLiteral("Open with the source pane hidden."));
    parser.addOption(previewOnly);

    parser.process(app);

    MainWindow window;
    if (parser.isSet(previewOnly))
        window.toggleEditorAction()->setChecked(false);
    window.show();

    const QStringList arguments = parser.positionalArguments();
    if (!arguments.isEmpty())
        window.openFile(arguments.first());

    return app.exec();
}