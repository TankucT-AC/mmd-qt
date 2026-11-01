# FileDocument

## 1. Class Overview

`FileDocument` owns the file a window is showing: it loads it, saves it, and
reports when another program changes it on disk. Watching covers both in-place
edits and the atomic-replace pattern (write a temporary file, then rename) used
by most editors — the file's *directory* is watched as well, so a replaced file
is noticed and the watch re-armed.

The class is deliberately GUI-free: it never prompts. Deciding what to do about
an external change is the window's job.

## 2. Project Structure and Dependencies

- Used by `app/MainWindow` (member `m_file`).
- Depends on Qt6 Core (`QFile`, `QFileInfo`, `QDir`, `QFileSystemWatcher`).

## 3. Class Hierarchy and Role

Derives from `QObject` for signals and parent ownership. `MainWindow` constructs it with itself as parent.

## 7. Signals

#### void externallyChanged()

The file changed on disk (edited, replaced or removed) and, for edits, its new
contents were read into `content()`. Not emitted for changes made through
`save()`. A slot typically reloads the editor, asking first if there are unsaved
edits.

## 9. Public Methods

#### bool load(const QString &path, QString *error = nullptr)

Reads the whole file as UTF-8, records it as the current document and starts
watching. On failure returns `false` and, if `error` is non-null, writes a
human-readable message there.

#### bool save(const QString &path, const QString &content, QString *error = nullptr)

Writes `content` as UTF-8 to `path`, makes it the current document and re-arms
the watch. Returns `false` with a message on failure. Writes through this method
do not emit `externallyChanged`.

#### void close()

Stops watching and forgets the current file (used by File → New).

#### QString filePath() const / QString content() const / bool exists() const

The current absolute path (empty when untitled), the last known file contents,
and whether the file currently exists on disk.

## 11. Ownership and Lifecycle

`QObject`; the `QFileSystemWatcher` is a value member. No raw pointers are
handed out. Parent-owned by the window.

## 12. Thread Safety

GUI-thread only: `QFileSystemWatcher` delivers its signals on the thread that
owns it, and `FileDocument` reacts synchronously (it re-reads the file inside the
slot).

## 14. Inter-Class Interactions

- `MainWindow::openFile` / `saveFile` drive it.
- `MainWindow` connects `externallyChanged` to `onExternalChange`, which applies the reload-versus-prompt policy.

## 16. Usage Example

```cpp
FileDocument file;
QObject::connect(&file, &FileDocument::externallyChanged, [&] { editor->setMarkdown(file.content()); });

QString error;
if (!file.load(QStringLiteral("/home/me/notes.md"), &error))
    qWarning("open failed: %s", qPrintable(error));
```