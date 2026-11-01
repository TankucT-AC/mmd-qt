# ImageResolver

## 1. Class Overview

`ImageResolver` turns the image reference in a markdown span (`![alt](path)`)
into a concrete local file path, resolved relative to the directory of the
markdown file — the behaviour readers expect. Remote URLs are out of scope: they
resolve to an empty path and the renderer shows a placeholder instead. It also
reports whether a path is an animated format the viewer can play.

## 2. Project Structure and Dependencies

- Used by `core/MarkdownBuilder` when building `MD_SPAN_IMG`.
- Depends on Qt6 Core (`QFileInfo`, `QDir`, `QUrl`).

## 3. Class Hierarchy and Role

A stateless helper: only static methods, no base, no instance state.

## 9. Public Methods

#### static QString ImageResolver::resolve(const QString &source, const QString &baseDir)

Returns an absolute local path for `source`. Relative references are joined onto
`baseDir` (or resolved against the current directory when `baseDir` is empty);
`file://` URIs are converted to local paths. Returns an empty string for remote
or unsupported schemes, and for an empty source.

#### static bool ImageResolver::isAnimated(const QString &path)

True when the path has an animated suffix the viewer handles. In v1 this is
`gif`; `MarkdownBuilder` uses it to emit an animated-image object instead of a
static image.

## 11. Ownership and Lifecycle

No ownership; both methods are pure.

## 12. Thread Safety

Stateless and thread-safe (only Qt value types and filesystem queries).

## 14. Inter-Class Interactions

- `MarkdownBuilder::build` calls `resolve()` for images and `isAnimated()` to choose between a static image and a GIF object.

## 16. Usage Example

```cpp
const QString path = ImageResolver::resolve(QStringLiteral("../img/plot.png"), QStringLiteral("/home/me/docs"));
const bool animated = ImageResolver::isAnimated(path);
```