// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("mmd"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QApplication::setOrganizationName(QStringLiteral("mmd-qt"));
    QApplication::setStyle(QStringLiteral("Fusion"));

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