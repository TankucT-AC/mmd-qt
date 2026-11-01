# mmd-qt reference documentation

Generated reference for the classes and entry point of mmd-qt. Each page covers
the public surface, ownership, threading and how the class fits into the app.

## Core (`src/core`)

| Page | Summary |
|---|---|
| [MarkdownBuilder.md](MarkdownBuilder.md) | Turns markdown source into a rendered `QTextDocument` using md4c. |
| [Theme.md](Theme.md) | Semantic theme definition: palette, markdown roles, syntax theme name; JSON round-trip. |
| [ThemeManager.md](ThemeManager.md) | Resolves Light/Dark/System/Custom into the active theme and emits on change. |
| [FontManager.md](FontManager.md) | Bundled/system fonts and the modular heading scale. |
| [ImageResolver.md](ImageResolver.md) | Resolves markdown image references to local paths and detects animation. |
| [FileDocument.md](FileDocument.md) | Loads, saves and watches the document file on disk. |
| [SystemThemeProvider.md](SystemThemeProvider.md) | Reads the desktop's own colour schemes (KDE `*.colors`). |

## Editor (`src/editor`)

| Page | Summary |
|---|---|
| [MarkdownEditor.md](MarkdownEditor.md) | The source pane: line-numbered `QPlainTextEdit` with theme support. |
| [MarkdownHighlighter.md](MarkdownHighlighter.md) | Markdown syntax highlighting for the source pane. |

## Viewer (`src/viewer`)

| Page | Summary |
|---|---|
| [MarkdownView.md](MarkdownView.md) | The preview pane: coalesced re-render, scroll preservation. |
| [CodeHighlighter.md](CodeHighlighter.md) | Highlights fenced code blocks per language via KSyntaxHighlighting. |
| [GifObjectHandler.md](GifObjectHandler.md) | Plays animated GIFs inside the document. |
| [CheckboxObjectHandler.md](CheckboxObjectHandler.md) | Draws task-list checkboxes for the active theme. |

## Application (`src/app`, `src`)

| Page | Summary |
|---|---|
| [MainWindow.md](MainWindow.md) | Composition: panes, menus, file handling, theme wiring. |
| [ScrollSync.md](ScrollSync.md) | Proportional two-way scroll synchronisation. |
| [main.md](main.md) | Startup sequence and command-line handling. |