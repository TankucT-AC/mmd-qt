# main.cpp

## A. Overview

The startup sequence of mmd-qt. It is a thin file by design: it configures the
`QApplication`, parses the command line, creates the single top-level
`MainWindow`, opens an optional file and hands control to the event loop. All
behaviour lives in the classes the window wires together, so nothing here needs
to be tested beyond an offscreen smoke run.

## B. Qt Application Setup

A `QApplication` is created because the app is Widgets-based. Before the window
is shown:

- `setApplicationName("mmd")`, `setApplicationVersion("0.1.0")`,
  `setOrganizationName("mmd-qt")` — identity used for settings and about text.
- `setStyle("Fusion")` — a consistent base style across desktops; the palette is
  then supplied by `ThemeManager`, so the look is identical on every platform.

## C. Command-Line Handling

`QCommandLineParser` is configured with:

| Option | Meaning |
|--------|---------|
| `--help` / `-h` | Usage text (added by `addHelpOption()`). |
| `--version` / `-v` | Version (added by `addVersionOption()`). |
| `--preview` | Open with the source pane hidden. |
| `[file]` | Positional: a markdown file to open. |

## D. Top-Level Object Creation

- `MainWindow window;` — the only top-level object. It constructs the panes,
  menus, theme manager and file document.

If `--preview` was given, the window's editor-toggle action is unchecked before
`show()` so the source pane starts hidden.

## E. Wiring and Connections

There is no wiring in `main.cpp` beyond the window: the window wires its own
collaborators. The only interactions are calling
`MainWindow::openFile()` with the positional argument (after `show()`, so errors
are reported against a visible window) and toggling the editor action for
`--preview`.

## F. Event Loop

`app.exec()` runs the event loop and returns the process exit code, which is
returned from `main`.

## G. Dependencies

- `QApplication`, `QCommandLineParser` — Qt6 Widgets.
- `app/MainWindow.h` — the application window (see MainWindow.md).

## Usage

```sh
mmd                       # empty window
mmd notes.md              # open a file
mmd --preview notes.md    # viewer-only
```