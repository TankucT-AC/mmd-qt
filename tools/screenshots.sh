#!/usr/bin/env sh
# Re-shoot the six README gallery screenshots into assets/.
#
# Usage: tools/screenshots.sh [path-to-binary]
#
# The binary defaults to out/gcc-release/src/total-markdown; build it first:
#
#   cmake --preset gcc-release && cmake --build --preset gcc-release
#
# Every shot launches the app with --screenshot, which opens a window of a
# fixed size, lets it settle, saves a grab and exits (a real window flashes on
# your desktop for about a second per shot). Each launch gets its own
# XDG_CONFIG_HOME/XDG_DATA_HOME/XDG_CACHE_HOME under a temporary directory and
# runs under the C.UTF-8 locale, so your real settings and image cache are
# never read or rewritten, every run starts from the shipped defaults, and the
# status bar reads English like the rest of the README.
#
# Framing per shot: the sidebar is hidden unless --sidebar is passed, and the
# source pane is hidden unless --editor is passed. The demo documents are
# generated here, one viewport each; only the two-pane and sidebar shots open
# the real feature-showcase.md from the repository, so the gallery shows the
# document a reader can actually open.

set -eu

repo=$(cd "$(dirname "$0")/.." && pwd)
bin=${1:-$repo/out/gcc-release/src/total-markdown}
size=1600x1000

if [ ! -x "$bin" ]; then
    echo "screenshots.sh: no executable at $bin" >&2
    echo "build it with: cmake --preset gcc-release && cmake --build --preset gcc-release" >&2
    exit 1
fi

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
export LC_ALL=C.UTF-8

# ---------------------------------------------------------------------------
# Demo documents. Each fits one viewport so no scrolling is needed.
# ---------------------------------------------------------------------------

cat > "$work/code.md" <<'EOF'
# Code highlighting

Fenced blocks are highlighted per language by KDE's KSyntaxHighlighting,
coloured from a palette matched to the active theme — switch to Dark under
View → Theme and this page restyles itself.

```cpp
// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QTimer>

/// One debounce timer per document: keystrokes coalesce into a single rebuild.
class DebouncedRenderer : public QObject {
    Q_OBJECT

public:
    explicit DebouncedRenderer(std::function<void()> rebuild, QObject *parent = nullptr)
        : QObject(parent), m_rebuild(std::move(rebuild))
    {
        m_timer.setSingleShot(true);
        m_timer.setInterval(120);
        connect(&m_timer, &QTimer::timeout, this, [this] { m_rebuild(); });
    }

    void schedule() { m_timer.start(); }

private:
    QTimer m_timer;
    std::function<void()> m_rebuild;
};
```

Hover the block for the copy button; one click puts its text on the
clipboard. Inline code such as `MarkdownBuilder::build()` is tinted the
same way, in every pane.

```python
def coalesce(keystrokes, window=0.12):
    """Rebuild once per burst of keystrokes, not once per key."""
    chunks = chunk_by_gaps(keystrokes, window)
    return [render(c) for c in chunks]
```
EOF

cat > "$work/math.md" <<'EOF'
# LaTeX math

Inline and display formulas are typeset by the bundled MicroTeX engine and
drawn as crisp, theme-coloured bitmaps. Euler's identity, $e^{i\pi} + 1 = 0$,
typesets right on the text line.

$$
\int_{-\infty}^{\infty} e^{-x^{2}}\,dx = \sqrt{\pi}
$$

A derivative in prose, $\frac{d}{dx}\left(x^{2}\right) = 2x$, reads inline,
and so does Pythagoras: $a^{2} + b^{2} = c^{2}$. Greek letters —
$\alpha$, $\beta$, $\Delta$, $\Omega$ — need no escaping.

$$
\hat{H}\psi = E\psi \qquad \mathbf{F}_{net} = \sum_{i=1}^{n} \mathbf{F}_{i}
$$

A formula that cannot be rendered shows its source text instead of a hole,
so a typo never leaves a blank in the page.
EOF

cat > "$work/gfm.md" <<'EOF'
# GitHub-Flavored Markdown

Tables, task lists, strikethrough and autolinks all render, and the HTML
tables found in real READMEs keep their markdown inside the cells.

| Shortcut | Action | Since |
|----------|--------|:-----:|
| `Ctrl+B` | Show/hide the sidebar | 0.1 |
| `F9` / `F10` | Show/hide a pane | 0.1 |
| `Ctrl+F` | Find in the focused pane | 0.2 |
| ~~`Ctrl+R`~~ | ~~Reload preview~~ Replaced by live rendering | — |

- [x] Parse CommonMark with md4c
- [x] Render GFM tables and task lists
- [x] Highlight fenced code per language
- [ ] Collaborative editing

> [!NOTE]
> Callouts from GitHub READMEs render in place, like this one.

**Bold**, *italic*, ***both***, `inline code`, ~~struck through~~ — the
usual emphasis marks all work, and bare URLs become links: visit
https://example.com or point a renderer at
https://github.com/mity/md4c directly.
EOF

cat > "$work/find.md" <<'EOF'
# Finding your way

Ctrl+F opens the find strip over the pane that has the keyboard focus.
Literal search wraps around, a counter shows where you are, and every hit
is highlighted in place — here, every occurrence of the word markdown.

The source pane and the preview keep their own search states, so a markdown
query in the editor does not disturb the reader's position in the rendered
markdown next to it.

Match case is one toggle away, F3 and Shift+F3 walk the matches, and Esc
closes the strip. Both panes decorate every hit at once, so nothing hides.

Markdown was designed for writing, and markdown parsing stays fast on big
documents. Markdown, markdown, markdown: the counter counts them all.
EOF

# ---------------------------------------------------------------------------
# Six shots, in README gallery order. --delay covers the render plus, for the
# math shot, MicroTeX's one-time font table initialisation.
# ---------------------------------------------------------------------------

shotdir=$work
shoot() {
    name=$1
    shift
    printf '>> %s\n' "$name"
    # A fresh state directory per shot: no layout or theme leaks from one
    # framing into the next.
    shotdir="$work/$(basename "$name" .png)"
    mkdir -p "$shotdir/config" "$shotdir/data" "$shotdir/cache"
    XDG_CONFIG_HOME="$shotdir/config" XDG_DATA_HOME="$shotdir/data" XDG_CACHE_HOME="$shotdir/cache" \
        "$bin" --screenshot "$repo/assets/$name" --size "$size" "$@"
}

shoot screenshot-two-pane.png  --theme light --editor "$repo/feature-showcase.md"
shoot screenshot-dark-code.png --theme dark "$work/code.md"
shoot screenshot-math.png      --theme light --editor --delay 1600 "$work/math.md"
shoot screenshot-sidebar.png   --theme light --sidebar "$repo/feature-showcase.md"
shoot screenshot-gfm.png       --theme light "$work/gfm.md"
shoot screenshot-find.png      --theme light --editor --find markdown "$work/find.md"

printf 'Done: %s\n' "$repo/assets/screenshot-"*.png
