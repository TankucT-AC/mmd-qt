# ImageFetcher

## 1. Class Overview

`ImageFetcher` loads remote images referenced by `http(s)` URLs, asynchronously
and with guard rails. The preview pane hands it a URL, gets on with its layout,
and receives the decoded image later through `ready()`.

It is built for a document like a project README, which routinely points at
shields.io badges and GitHub-hosted banners and icons. The fetcher never blocks:
the request goes out in the background, the decoded image is kept in a memory
cache, and the original bytes are kept on disk, so a document opened twice — or
two documents sharing a badge — downloads the bytes once. The second open is
instant and works offline.

Security posture, because a markdown file can come from anywhere:

- `http` and `https` only.
- Loopback and private network space are refused (the SSRF guard). A name is
  resolved first and every answer is checked, so a hostname pointing at
  `10.0.0.1` is refused just like the literal address.
- Redirects are followed by hand, up to a limit, and every hop is re-checked
  against the same rules.
- The response is capped in bytes and carries a transfer timeout.
- The payload, not the content type, decides whether something is an image —
  some hosts serve SVG as `text/plain`.

## 2. Project Structure and Dependencies

- Owned by `viewer/MarkdownView`; the view connects `ready()`/`failed()` and
  stores decoded images in its shared resource cache.
- Depends on Qt6 Network (`QNetworkAccessManager`) and Qt6 Svg
  (`QSvgRenderer`), plus Qt6 Gui for decoding and `QImageReader`.

## 3. Class Hierarchy and Role

Derives from `QObject` (signals only, no widgets). It is a concrete service: one
instance per view, living as long as the view.

## 4. Signals

#### void ready(const QUrl &url, const QImage &image)

The image for `url` is decoded and ready to draw. Emitted asynchronously, except
when the URL was already in the memory or disk cache — then it fires during the
`fetch()` call itself.

#### void failed(const QUrl &url)

`url` will not produce an image (refused, unreachable, not an image, too large,
or timed out). Emitted once per URL until `clearFailed()`.

## 9. Public Methods

#### void setEnabled(bool enabled) / bool isEnabled() const

When disabled, `fetch()` does nothing and nothing touches the network.

#### void setCacheDirectory(const QString &path)

Overrides the disk cache directory. The default is
`<user cache>/mmd-qt/remote-images`. Tests point it at a temporary directory.

#### void setRequestTimeoutMs(int ms) / setMaximumBytes(qint64) / setMaximumRedirects(int) / setConcurrentRequestLimit(int) / setPrivateNetworkAllowed(bool) / setDevicePixelRatio(qreal)

Guard-rail and behaviour knobs. Production code keeps the defaults (10 s
timeout, 4 MB cap, 3 redirects, 4 concurrent requests, private network refused,
ratio 1). Tests shrink them, or allow loopback so a fixture server can be
reached.

#### QImage cachedImage(const QUrl &url)

A synchronous cache read: memory first, then disk. An empty image means "not
cached". Returns the same image `ready()` would deliver.

#### void fetch(const QUrl &url)

Starts a background load. A URL already in flight is coalesced (no second
request); a URL that already failed stays failed until `clearFailed()`, so a
relayout never turns one missing file into a request storm. Nothing is emitted
while disabled.

#### bool isFetching(const QUrl &url) const

True while a request for `url` is queued or in flight.

#### void clearFailed()

Forgets past failures so their URLs can be requested again, e.g. when the
network comes back.

## 10. Internal State and Algorithms

- One `Request` (a small `QObject`) per in-flight load, keyed by the URL the
  caller asked for. Redirect hops update the request's `current` URL without
  changing the key, so a redirect chain still reports `ready()`/`failed()` for
  the original URL.
- Decoding: SVG is rasterised through `QSvgRenderer` at the intrinsic size times
  the device pixel ratio; static raster formats go through `QImageReader`. An
  animated GIF decodes as its first frame — remote animation is out of scope in
  v1.
- Disk cache: one `<sha256>.data` file plus a `<sha256>.data.meta` holding the
  content type and the fetch time. Entries older than a week are discarded on
  the next read.

## 11. Ownership and Lifecycle

Owns its `QNetworkAccessManager` and its `Request` objects. The destructor
detaches its callbacks from every reply before aborting it, so a teardown cannot
re-enter `fail()` and emit into a half-destroyed owner.

## 12. Thread Safety

GUI-thread only. The network work happens in Qt's own threads; the fetcher's
state (caches, request table) is touched only from the object's thread.

## 14. Inter-Class Interactions

- `viewer/MarkdownView`: creates one fetcher, wires `ready`/`failed`, and calls
  `fetch()` from `PreviewDocument::loadResource` when a remote image is met
  during layout.
- `core/ImageResolver::isRemote` classifies a reference before it is handed over.

## 16. Usage Example

```cpp
ImageFetcher fetcher;
connect(&fetcher, &ImageFetcher::ready, this, [](const QUrl &url, const QImage &image) {
    // draw `image` where `url` is referenced
});
fetcher.fetch(QUrl(QStringLiteral("https://img.shields.io/badge/Qt-6.5-41CD52")));
```
