# mmd-qt

A fast, native two-pane Markdown viewer and editor built with **Qt 6 Widgets**.
No Electron, no web engine, no QML — one monolithic desktop application that
opens instantly and re-renders as you type.

```
┌───────────────────────────┬───────────────────────────┐
│  Markdown source          │  Rendered preview         │
│  (syntax highlighted)     │  (CommonMark + GFM)       │
└───────────────────────────┴───────────────────────────┘
```

## Features

- **Two panes** — editable source on the left, live preview on the right.
  Each pane can be hidden independently (`F9` / `F10`, or the View menu).
- **Live preview** — edits are rendered immediately; a burst of keystrokes is
  coalesced into a single rebuild, and the scroll position is preserved.
  The two panes scroll in step.
- **Beautiful typography** — bundled [Inter](third_party/fonts/inter) for prose
  and [JetBrains Mono](third_party/fonts/jetbrains-mono) for code, switchable to
  the system fonts.
- **Syntax highlighting** — fenced code blocks are highlighted by language via
  KDE's KSyntaxHighlighting, and markdown itself is highlighted in the source
  pane.
- **Images and animations** — images resolve relative to the document; animated
  GIFs play in place.
- **Themes** — light, dark and *follow the system* (which reads the desktop's
  own colour scheme — e.g. a KDE Plasma scheme, as Dolphin does — and falls back
  to the platform scheme). Themes are data: any theme can be loaded from JSON,
  and discovered system schemes are listed in the menu.
- **GFM** — tables, task lists, strikethrough, autolinks and footnotes.

Curious what it looks like? Open [`feature-showcase.md`](feature-showcase.md)
— one document that exercises every feature above, with real image assets next
to it.

## Building

Requirements:

- CMake ≥ 3.24 and Ninja
- A C++20 compiler (GCC 13+, Clang 16+)
- Qt 6.5+ (Widgets, Test)
- KDE Frameworks 6 — **KSyntaxHighlighting**

```sh
cmake --preset gcc-debug
cmake --build --preset gcc-debug
ctest --preset gcc-debug
./out/gcc-debug/src/mmd
```

A release build with a second compiler is one more step:

```sh
cmake --preset clang-release && cmake --build --preset clang-release && ctest --preset clang-release
```

## Layout

```
src/app/      MainWindow, scroll synchronisation
src/core/     markdown -> QTextDocument, themes, fonts, image paths
src/editor/   the source pane: text editor + markdown highlighter
src/viewer/   the preview pane + code highlighting + animated images
tests/        one QTest binary per module, run through CTest
third_party/  vendored md4c (MIT) and bundled fonts (OFL)
```

Dependencies point one way: `app → editor/viewer → core`. See
[STYLEGUIDE.md](STYLEGUIDE.md) for the architecture and the conventions we
follow, and [CONTRIBUTING.md](CONTRIBUTING.md) for the process and the
licensing rules before you send a patch.

## License

GPL-3.0-or-later. Bundled third-party components keep their own licenses;
see [CONTRIBUTING.md](CONTRIBUTING.md) for the full list.