# MarkdownBuilder

## 1. Class Overview

mmd-qt renders markdown by converting the source directly into a `QTextDocument`
rather than to HTML. `MarkdownBuilder` is that converter, and it is the heart of
the preview pipeline.

It drives the [md4c](https://github.com/mity/md4c) parser (the same parser Qt's
own markdown import builds on) through its callback interface and writes the
result into a target document using semantic styles. The class is a pure
function of `(markdown, RenderStyle)`: it holds no widget, no event loop and no
global state, so it can be exercised in tests with nothing but a `QTextDocument`.

## 2. Project Structure and Dependencies

- Included and called by `viewer/MarkdownView` (`MarkdownBuilder::build` on every render).
- Depends on Qt6 Gui (`QTextDocument`, `QTextCursor`, `QTextTable`, `QTextList`) and on the vendored md4c C library (`third_party/md4c`).
- Uses `core/ImageResolver` to resolve image references and `core/Theme`/`core/FontManager` through `RenderStyle`.
- Build requirement: link `md4c` (see `third_party/CMakeLists.txt`); md4c headers are on the public include path of the `md4c` target.

## 3. Class Hierarchy and Role

`MarkdownBuilder` is a plain class with no base and no `Q_OBJECT`. It is a
stateless façade: the real work is done by an internal builder object inside the
translation unit, which implements the md4c callbacks and collects the parser
output. The façade only exists to give the rest of the app a stable, minimal
entry point.

## 5. Enumerations

| Value | Integer | Description |
|-------|---------|-------------|
| `BlockKind::None` | 0 | Default block kind; a normal paragraph or unmarked block. |
| `BlockKind::Heading` | 1 | A heading block (`#` … `######`). |
| `BlockKind::Code` | 2 | A fenced or indented code block. |
| `BlockKind::Quote` | 3 | Reserved for quote-marked blocks. |
| `BlockKind::Rule` | 4 | A horizontal rule. |

`BlockKind` is stored on each block's `QTextBlockFormat` under `kindKey` so later
passes (notably `CodeHighlighter`) can find blocks without re-parsing.

| Value | Description |
|-------|-------------|
| `PropertyKey::kindKey` | `BlockKind` value stored on a block format. |
| `PropertyKey::headingLevelKey` | Heading level (1–6) stored on a heading block format. |
| `PropertyKey::languageKey` | The info-string language of a code block, e.g. `cpp`. |
| `PropertyKey::gifPathKey` | Filesystem path of an animated image stored on its object char format. |
| `PropertyKey::imageAltKey` | Alt text of a remote image, kept on the image format so a failed download can fall back to its chip. |
| `PropertyKey::sourceLineKey` | Zero-based source line a block was built from; what click-to-source resolves. |

| Constant | Value | Description |
|----------|-------|-------------|
| `gifObjectType` | `QTextFormat::UserObject + 1` | Object type used for animated images; a `QTextObjectInterface` handler is registered for it. |

## 9. Public Methods

#### bool build(QTextDocument *target, const QString &markdown, const RenderStyle &style)

Parses `markdown` and rebuilds `target` as a rendered document. The document is
cleared first and filled in place. Returns `false` only when the parser itself
fails; it does not throw and does not touch any GUI state. The target needs no
object handlers and no layout work to be attached — the view builds into a
detached document and swaps it in once the text is complete.

Remote `http(s)` references are handled according to `RenderStyle`: with
`remoteImagesEnabled` set the builder emits a `QTextImageFormat` named by the
URL (plus the alt text under `imageAltKey`) and the viewer downloads it; with
the flag clear — or for a reference that is not an image the viewer can load —
the alt text becomes an inline chip, exactly as before. The builder itself
never touches the network.

## 11. Ownership and Lifecycle

`target` is not owned: the caller keeps the `QTextDocument` alive and owns it
(for the preview pane that owner is `MarkdownView`, the `QTextBrowser` itself).
`RenderStyle` is passed by const reference and copied internally; no pointer in
it outlives the call except `baseDir`, a value.

## 12. Thread Safety

Pure and reentrant with respect to *distinct* documents, but the md4c parser
state and the target document are not synchronised. In practice it is called on
the GUI thread only; a future off-thread render would construct a fresh document
in the worker and hand it back.

## 14. Inter-Class Interactions

- `MarkdownView` calls `build()` and then `CodeHighlighter::highlight()` on the result.
- `MarkdownView` registers a `GifObjectHandler` for `gifObjectType`, which the builder emits for animated images.
- `MarkdownView::sourceLineAt` reads `sourceLineKey` back off a block, which is how a click in the preview finds the line to put the editor's caret on.
- `core/HtmlFragment` tokenizes the raw HTML md4c hands over as `MD_BLOCK_HTML` / `MD_SPAN_HTML`.

## 15. HTML Rendering

md4c passes HTML through as raw text, the CommonMark way. `MarkdownBuilder`
renders it with a tolerant walk (`renderHtmlFragment`) rather than a second
markdown pass:

- Block containers are recognized by name. A container whose closing tag is in
  the same HTML block is rendered by running its inner markup back through the
  builder in a scratch document and pasting the result in with the container's
  alignment — which is why a `<td>` holding markdown comes out the way GitHub
  shows it (bold, lists, images and all).
- A block container that spans several markdown blocks — `<div align="center">`
  or `<p align="right">` with a blank line inside, where CommonMark ends the
  HTML block — is remembered on a stack and its alignment is applied to every
  block until the closing tag arrives.
- Block-level HTML tables are lifted out of the source before parsing
  (`extractHtmlTables`): with blank lines between their parts they would
  otherwise reach the builder as a scatter of fragments. Each table is replaced
  by a one-line marker that the block renderer expands into a real `QTextTable`,
  keeping the source line count so click-to-source stays correct.
- Inside a container, markdown is re-parsed by a nested `build()` call; SVG
  resources the inner build rasterised are copied over (`copyHtmlResources`)
  because a document fragment carries formats, not resources.
- Image widths come from the HTML `width` attribute and are parsed apart
  (`parseHtmlLength`): a bare number is pixels, capped to
  `RenderStyle::maxImageWidth`, while a percentage is a fraction of the text
  column the pane currently has (`RenderStyle::contentWidth`, falling back to
  `maxContentWidth`). Reading `"100%"` as the digits `"100"` used to draw a
  full-width banner at 100 px. Qt 6.12 ignores `QTextImageFormat::maximumWidth`
  when drawing, so the width is resolved to pixels at build time; the view
  re-renders when the column changes, so a hero keeps fitting an open editor
  pane instead of overflowing it.
- A scaled raster image is resampled *smoothly* once and handed over as a
  resource (`QImage::scaled(..., Qt::SmoothTransformation)`); the text layout
  paints a scaled image without interpolation, so a big screenshot looked rough
  left to it.
- An HTML image block uses a plain block format, not the paragraph one. The
  122% line height meant for prose stretched an image's line by a fifth of the
  image, so every hero left a visible gap below it.
- Every image format inside a link (`[![badge](pic.png)](url)`, `<a><img></a>`)
  carries the anchor on its own char format. Hit tests read the format under
  the cursor, so without it a badge link never shows the hand cursor and a
  click on it does nothing.

## 16. Usage Example

```cpp
RenderStyle style;
style.theme = ThemeDefinition::dark();
style.baseDir = QStringLiteral("/home/user/notes");

QTextDocument document;
MarkdownBuilder::build(&document, sourceText, style);
```