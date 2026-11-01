# Styleguide: how mmd-qt is built

This guide is about the **architecture and process** of the project, not about
any single widget. It is a description of what actually runs, with links to the
real files. For the QPainter fundamentals kept from an earlier project, see
[TUTORIAL.md](TUTORIAL.md); for rules you must follow when contributing, see
[CONTRIBUTING.md](CONTRIBUTING.md).

## 1. Map of the project

```
src/core/     parsing and data: markdown -> QTextDocument, themes, fonts, files
src/editor/   the source pane: text editor + markdown highlighter
src/viewer/   the preview pane: renderer + code highlighting + animated images
src/app/      composition: the window, the scroll link
tests/        one QTest binary per module, run through CTest
third_party/  vendored md4c (MIT) and bundled fonts (OFL)
```

Dependencies point one way: `app → viewer/editor → core`. **`core` includes
nothing from the layers above it.** `core/MarkdownBuilder` does not know a
widget exists; that is why it is testable without a GUI.

## 2. Widgets only

The UI is plain Qt Widgets: `QPainter`, `QTextDocument`, `QPlainTextEdit`,
`QTextBrowser`. No QML, no Qt Quick, no web engine. This is a hard rule — see
CONTRIBUTING.md — and it is what keeps the binary small and the startup instant.

## 3. Signals are the only glue

Layers do not call each other's methods when they live in different layers; they
emit signals and the wiring lives in one place, `src/app/MainWindow.cpp`:

```cpp
connect(m_editor, &QPlainTextEdit::textChanged, this, [this] {
    m_viewer->setMarkdown(m_editor->toPlainText());
});
connect(m_file, &FileDocument::externallyChanged, this, &MainWindow::onExternalChange);
```

Each class is then testable on its own — the tests drive `MarkdownBuilder`,
`FileDocument`, `ThemeManager` directly, with no window in sight.

## 4. Rendering is a pure function of state

`MarkdownBuilder::build(document, source, style)` takes a source string and a
style and produces a document. It reads no globals, mutates no shared state, and
rebuilds **in place** rather than returning a new document. Consequences:

- Live preview is cheap: a rebuild is just another call.
- The document instance stays valid, so the GIF handler registration survives.
- Tests assert on the resulting `QTextDocument` directly (`tests/test_markdown.cpp`).

`RenderStyle` bundles everything the renderer needs — theme, fonts, base
directory, image cap — so adding a theme or a font set never touches the
renderer's signature.

## 5. Incremental rendering, honestly

`MarkdownView::setMarkdown()` records the source and arms a single-shot timer.
Further calls inside the window are absorbed, so a burst of keystrokes costs one
rebuild, not one per key. `renderInterval == 0` renders synchronously and is
what the tests use.

Re-rendering rebuilds the whole document. That is deliberate (KISS): md4c parses
hundreds of kilobytes in milliseconds, and a block-level diff would add state
for a problem we have not measured. The scroll position is preserved by
anchoring on the top block, so the viewport does not jump while you type. If a
very large document ever proves slow, the next step is to parse off the GUI
thread — not to diff.

The rendered column is capped (`RenderStyle::maxContentWidth`) and centred, like
Zed: on a wide window the text keeps a comfortable line length instead of
stretching edge to edge. `MarkdownView` achieves this with equal viewport
margins, recomputed on resize.

## 6. Themes: data, not code

All colors live in `ThemeDefinition` (`src/core/Theme.h`) and are named by role
— `heading`, `link`, `codeBackground`, `quoteBar`, plus a separate `SyntaxColors`
set for code — never by shade. Widgets and the renderer must not write literal
`QColor`s. `ThemeManager` resolves the active `ThemeDefinition` from a *mode*
(Light / Dark / System / Custom):

- **System** follows the desktop: it first looks for the desktop's own colour
  scheme (the KDE `*.colors` files Dolphin uses) via `SystemThemeProvider`, and
  falls back to `QStyleHints::colorScheme()` when none is present. That provider
  is plain INI parsing — no KDE dependency — so the app stays portable; on other
  desktops it simply reports nothing.
- **Custom** loads a theme from JSON (`ThemeDefinition::fromJson`) or from a
  discovered system scheme, which is the extension point for arbitrary themes.

The mode the user picked is remembered across runs (`QSettings`, written by
`MainWindow` on every mode change and reloaded on startup).

Because color is a value type, a theme swap is just `setRenderStyle(newTheme)`
followed by a rebuild — no code paths fork on "which theme".

### Code colors

Fenced code is highlighted by `CodeHighlighter`, which maps KSyntaxHighlighting
text styles onto the theme's `SyntaxColors` (a GitHub-like palette). We do **not**
use KSyntaxHighlighting's own themes for color: those ship a very different
palette that would clash with the rest of the app. The KF6 theme is still set so
bold/italic styles resolve, but the foreground always comes from
`ThemeDefinition::syntax`.

## 7. Typography

Typography is centralized in `FontManager` (`src/core/FontManager.h`): Inter for
prose, JetBrains Mono for code, both OFL and compiled into the binary as Qt
resources. A `FontSet::System` option falls back to the platform fonts. Heading
sizes follow a modular scale (base 12 pt, ratio 1.2), so the hierarchy stays
proportional if the base changes — no magic numbers scattered across files.

## 8. Images, HTML inserts and animations

**Animated GIF** plays in place (`GifObjectHandler`, a `QTextObjectInterface`
driving a `QMovie`). **Static images** (PNG, JPEG, WebP) are drawn by the text
layout directly. **SVG** is resolution-independent, so it is rasterised on the
fly with `QSvgRenderer` (Qt6::Svg) at the *device* pixel size and with
`Antialiasing` enabled, then handed to the document as an image resource — that
keeps vector art crisp and smooth instead of showing stair-stepped edges. Any
other image is scaled down to `RenderStyle::maxImageWidth`.

**HTML inserts.** Real READMEs are full of HTML, so the renderer understands a
small, tolerant subset — enough to make a GitHub README look right, without a
web engine (`QTextDocument::setHtml` is deliberately *not* used: it drags in its
own fonts and colours and would break the theme). `HtmlFragment` tokenizes the
raw HTML; `MarkdownBuilder` renders:

| Construct | Rendering |
|---|---|
| `<div align="center">` … `</div>` | every block between them is centred (a persistent context, since the tags arrive as separate HTML blocks) |
| `<details>` / `<summary>` | a marked, bold title line (a `QTextDocument` cannot collapse) |
| `<table>` / `<tr>` / `<th>` / `<td>` | a real `QTextTable` |
| `<img>` (remote) | an offline **chip** with the `alt` text — no network |
| `<b>`, `<i>`, `<code>`, `<a>`, `<sub>`, `<sup>`, `<br>`, `<kbd>` | the matching span styles (`<kbd>` becomes a chip) |
| `> [!NOTE]` … `[!CAUTION]` | a coloured callout bar and label |
| anything unknown | kept visible as raw text |

**Code boxes** get a **copy button**: hovering reveals it in the top-right
corner, and clicking copies the block's plain text (`MarkdownView` draws it as a
`paintEvent` overlay and hit-tests in the mouse handlers).

## 9. Syntax highlighting

Two independent highlighters, both driven by the theme:

- **Markdown source** (`editor/MarkdownHighlighter`, a `QSyntaxHighlighter`):
  a lightweight visual aid for the person typing — headings, fences, spans,
  links — with a block-state machine for multi-line fences.
- **Fenced code** (`viewer/CodeHighlighter`, a KF6 `AbstractHighlighter`): each
  fence is highlighted with its own language definition and a reset state, since
  one document can mix C++, Python and Bash. The language comes from the block
  property the builder set from the fence's info string.

One gotcha is worth recording: a `QSyntaxHighlighter` pass that runs *during*
`setPlainText` writes to a layout that is discarded afterwards. Programmatic
text replacement therefore goes through `MarkdownEditor::setMarkdown()`, which
re-highlights explicitly.

## 10. Files and the outside world

`FileDocument` (`src/core/FileDocument.h`) owns the file on disk. It watches the
file *and its directory*, so it notices the atomic-replace pattern (write-temp,
rename) most editors use. It never prompts; deciding what to do about an
external change is `MainWindow`'s job, which is why the policy — reload
silently, or ask before discarding unsaved edits — lives in one slot.

## 11. Testing

Format: **one QTest binary per module**, registered in CTest

```sh
ctest --preset gcc-debug                    # everything
./out/gcc-debug/tests/test_markdown -functions   # list cases
```

What is tested, and how:

- **pure logic** — the renderer (`test_markdown`), themes (`test_theme`), fonts
  (`test_fonts`), file loading and watching (`test_file`) — by asserting on the
  produced values, not on pixels.
- **widgets that can be driven head-less** — the editor highlighter
  (`test_editor`), the preview and scroll link (`test_view`), the GIF handler
  (`test_gif`) — using `QT_QPA_PLATFORM=offscreen`.
- **the composed window** — `test_app` builds a real `MainWindow` and exercises
  open/save/external-change end to end.
- **GUI painting is not unit-tested.** It is covered by the offscreen smoke run
  below, exactly as CONTRIBUTING.md describes.

The order is red → green: write the failing test, then the implementation. The
history of this repository is a sequence of such steps.

## 12. The pipeline for any change

```sh
cmake --build --preset gcc-debug && ctest --preset gcc-debug   # tests

clang-format -i <files>                                        # formatting

QT_QPA_PLATFORM=offscreen ./out/gcc-debug/src/mmd sample.md    # smoke, no monitor

cmake --preset clang-release && cmake --build --preset clang-release && ctest --preset clang-release
```

Commits are small and conventional (`feat:`, `fix:`, `refactor:`, `docs:`); one
logical step per commit, and the tree builds and passes after every one.

## 13. Review checklist

Drawn from bugs actually hit while building this project:

- [ ] **Repo opens**: every `readDatagram`-class call — `QFile::open`,
      `QFile::write` — has its result checked and surfaces an error.
- [ ] **`QSyntaxHighlighter` after programmatic text**: call the explicit
      re-highlight path (`setMarkdown`), not a bare `setPlainText`.
- [ ] **Resources in a static library**: anything `qrc`-compiled needs
      `Q_INIT_RESOURCE`, or the linker drops it (see `FontManager`).
- [ ] **New `Q_OBJECT` files are in CMakeLists** (`CMAKE_AUTOMOC` cannot invent
      a target).
- [ ] **Colors come from the theme**, never a literal `QColor`.
- [ ] **No unbounded `wait()`**, no GUI work off the GUI thread.
- [ ] **Scroll/layout values guarded** against a zero range before dividing.
- [ ] **License headers intact** and new files carry SPDX lines.

## 14. Where to look next

| To understand | Read |
|---|---|
| How markdown becomes a document | `src/core/MarkdownBuilder.h`, then the tests |
| Themes and typography | `src/core/Theme.h`, `src/core/FontManager.h` |
| Live preview and scroll sync | `src/viewer/MarkdownView.cpp`, `src/app/ScrollSync.cpp` |
| Code highlighting | `src/viewer/CodeHighlighter.cpp` |
| Files and watching | `src/core/FileDocument.cpp` |
| How it is all wired | `src/app/MainWindow.cpp` |
| QPainter fundamentals | [TUTORIAL.md](TUTORIAL.md) |