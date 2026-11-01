# Contributing to mmd-qt

Thank you for considering a contribution. This document sets the rules that
keep the project legally clean and the codebase consistent. Read it before
your first commit.

## Licensing rules (mandatory)

mmd-qt is licensed under **GPL-3.0-or-later** (see [LICENSE](LICENSE)). The
repository also ships third-party code with its own licenses:

| Component | Location | License |
|---|---|---|
| md4c parser | `third_party/md4c/` | MIT (Martin Mitaš) |
| Inter typeface | `third_party/fonts/inter/` | SIL OFL 1.1 |
| JetBrains Mono | `third_party/fonts/jetbrains-mono/` | SIL OFL 1.1 |
| `.clang-format` | repository root | MIT (KDE ECM, Christoph Cullmann, Gernot Gebhard) |
| KF6 SyntaxHighlighting | external dependency | MIT (KDE) |

**The rules:**

1. **Never delete or weaken license notices.** When editing a file that
   carries a license header (SPDX or plain text), keep the header intact. This
   applies in particular to KDE, MIT and OFL notices anywhere in the tree.
   When updating a vendored file from upstream, carry the new upstream notice
   over verbatim.
2. **Edits are made in the contributor's name.** Commits are authored by the
   human contributor who reviewed and approved the change; git authorship must
   always identify a real person responsible for the change.
3. **AI-assisted commits are allowed, but the contributor is fully
   responsible** for every change in them, as if they had written it by hand.
   Review AI-generated diffs with the same scrutiny as a stranger's patch.
4. **No AI co-authorship.** Do not add `Co-Authored-By:` trailers crediting AI
   tools, and do not credit AI tools as authors or copyright holders.
5. **New files** must start with the SPDX headers:

   ```
   // SPDX-FileCopyrightText: 2026 <your name> <email>
   // SPDX-License-Identifier: GPL-3.0-or-later
   ```

   Use `SPDX-License-Identifier: MIT` only inside `third_party/` when porting
   MIT-licensed upstream code, and keep the upstream copyright line.

## Architecture constraints

- **QWidgets only.** The UI is plain Qt Widgets (`QPainter`, `QTextDocument`,
  model/view). Do not introduce QML / Qt Quick dependencies; such changes will
  be rejected.
- The application is **monolithic**: one executable, one internal static
  library (`mmd_core`), no plugins.
- Dependencies point one way: `app → editor/viewer → core`. `core` must not
  include anything from the layers above it.
- Signals are the only glue between layers (see [STYLEGUIDE.md](STYLEGUIDE.md)).

## Development process

1. **TDD.** Write the failing test first (red), then the implementation
   (green). One QTest binary per module under `tests/`, registered in CTest.
2. Build and run everything:

   ```sh
   cmake --preset gcc-debug && cmake --build --preset gcc-debug
   ctest --preset gcc-debug
   ```

3. Format the files you touched (the `.clang-format` config is authoritative):

   ```sh
   clang-format -i <files>
   ```

4. GUI changes need an offscreen smoke run:

   ```sh
   QT_QPA_PLATFORM=offscreen ./out/gcc-debug/src/mmd &
   # exercise the feature, then kill; no crashes or warnings in the log
   ```

5. Before merging, verify with the second compiler:

   ```sh
   cmake --preset clang-release && cmake --build --preset clang-release && ctest --preset clang-release
   ```

## Commits

- Small, self-contained commits with conventional prefixes: `feat:`, `fix:`,
  `refactor:`, `docs:`, `chore:`, `test:`.
- One logical step per commit; the tree must build and pass tests after every
  commit.
- By committing you confirm that you had the right to submit the change and
  that it is licensed under GPL-3.0-or-later.
