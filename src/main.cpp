// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/MainWindow.h"
#include "core/MathRenderer.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QIcon>
#include <QTimer>

// Multi-size app icon from the bundled resources, so the window/taskbar gets a
// crisp mark at every size instead of one scaled bitmap.
static QIcon appIcon()
{
    QIcon icon;
    for (int size : {16, 24, 32, 48, 64, 128, 256})
        icon.addFile(QStringLiteral(":/mmd/logo/logo-%1.png").arg(size));
    return icon;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("total-markdown"));
    QApplication::setApplicationVersion(QStringLiteral("0.2.0"));
    QApplication::setOrganizationName(QStringLiteral("total-markdown"));
    QApplication::setStyle(QStringLiteral("Fusion"));
    QApplication::setWindowIcon(appIcon());

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Total Markdown — a two-pane Markdown viewer and editor"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("file"), QStringLiteral("Markdown file to open."), QStringLiteral("[file]"));

    QCommandLineOption previewOnly(QStringLiteral("preview"), QStringLiteral("Open with the source pane hidden."));
    parser.addOption(previewOnly);

    // The screenshot harness behind tools/screenshots.sh: open a window of a
    // fixed size, let it settle, save a grab and exit. Hidden from --help, the
    // way --preview is not, because only scripts call it.
    QCommandLineOption screenshotOption(QStringLiteral("screenshot"),
        QStringLiteral("Render the window and save it to <file>, then exit."), QStringLiteral("file"));
    QCommandLineOption themeOption(QStringLiteral("theme"),
        QStringLiteral("Theme mode for this run: light, dark or system."), QStringLiteral("mode"));
    QCommandLineOption sizeOption(QStringLiteral("size"),
        QStringLiteral("Window size as WxH (default 1600x1000)."), QStringLiteral("size"));
    QCommandLineOption editorOption(QStringLiteral("editor"), QStringLiteral("Show the source pane beside the preview."));
    QCommandLineOption sidebarOption(QStringLiteral("sidebar"), QStringLiteral("Show the sidebar with the Outline tab."));
    QCommandLineOption findOption(QStringLiteral("find"),
        QStringLiteral("Open the find strip over the source pane with this query."), QStringLiteral("query"));
    QCommandLineOption delayOption(QStringLiteral("delay"),
        QStringLiteral("Milliseconds to wait before the grab (default 900)."), QStringLiteral("ms"));
    parser.addOption(screenshotOption);
    parser.addOption(themeOption);
    parser.addOption(sizeOption);
    parser.addOption(editorOption);
    parser.addOption(sidebarOption);
    parser.addOption(findOption);
    parser.addOption(delayOption);

    parser.process(app);

    MainWindow window;
    if (parser.isSet(previewOnly))
        window.toggleEditorAction()->setChecked(false);

    if (parser.isSet(sizeOption)) {
        const QString text = parser.value(sizeOption);
        const int x = static_cast<int>(text.indexOf(QLatin1Char('x')));
        bool widthOk = false;
        bool heightOk = false;
        const int width = text.left(x < 0 ? 0 : x).toInt(&widthOk);
        const int height = text.mid(x < 0 ? text.size() : x + 1).toInt(&heightOk);
        if (widthOk && heightOk && width > 0 && height > 0)
            window.resize(width, height);
    }

    if (parser.isSet(themeOption)) {
        const QString name = parser.value(themeOption).toLower();
        if (name == QLatin1String("light"))
            window.themeManager()->setMode(ThemeManager::Mode::Light);
        else if (name == QLatin1String("dark"))
            window.themeManager()->setMode(ThemeManager::Mode::Dark);
        else if (name == QLatin1String("system"))
            window.themeManager()->setMode(ThemeManager::Mode::System);
    }

    if (parser.isSet(editorOption))
        window.toggleEditorAction()->setChecked(true);
    if (parser.isSet(sidebarOption))
        window.setSidebarVisible(true);
    else if (parser.isSet(screenshotOption))
        window.setSidebarVisible(false); // frame the panes without the dock

    window.show();

    const QStringList arguments = parser.positionalArguments();
    if (!arguments.isEmpty()) {
        window.openFile(arguments.first());
    } else {
        // Nothing to render yet. Bring the formula renderer's one-time font
        // table initialisation up behind the first frame, so the first file
        // the reader opens does not pay for it mid-render.
        QTimer::singleShot(0, &window, [] {
            MathRenderer::instance().isAvailable();
        });
    }

    if (parser.isSet(findOption)) {
        const QString query = parser.value(findOption);
        // Once the window is on screen and the editor owns the keyboard focus,
        // so the strip opens over the source pane rather than the preview.
        QTimer::singleShot(120, &window, [&window, query] { window.openFind(query); });
    }

    if (parser.isSet(screenshotOption)) {
        const QString target = parser.value(screenshotOption);
        bool ok = false;
        const int delay = parser.value(delayOption).toInt(&ok);
        QTimer::singleShot(ok && delay > 0 ? delay : 900, &window, [&window, target] {
            window.grab().save(target);
            QApplication::quit();
        });
    }

    return app.exec();
}