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

## 16. Usage Example

```cpp
RenderStyle style;
style.theme = ThemeDefinition::dark();
style.baseDir = QStringLiteral("/home/user/notes");

QTextDocument document;
MarkdownBuilder::build(&document, sourceText, style);
```