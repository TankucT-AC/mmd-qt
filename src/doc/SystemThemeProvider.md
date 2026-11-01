# SystemThemeProvider

## 1. Class Overview

`SystemThemeProvider` lets mmd-qt follow the desktop's own colours the way
Dolphin does, without giving up portability. It reads KDE colour-scheme files
(the `*.colors` files Plasma apps use) from the standard locations and turns
them into `ThemeDefinition` values.

A `.colors` file is plain INI, so this adds **no KDE dependency** — it is parsed
with a small hand-written reader. On desktops and platforms where those files do
not exist, every method simply reports nothing and the app falls back to its
built-in themes and to `QStyleHints`.

## 2. Project Structure and Dependencies

- Used by `core/ThemeManager` (System mode) and `app/MainWindow` (the System Schemes menu).
- Depends on Qt6 Core (`QSettings` is *not* used; `QFile`, `QDir`, `QStandardPaths`, `QTextStream`) and `core/Theme`.
- No build requirement beyond Qt.

## 3. Class Hierarchy and Role

A stateless helper: only static methods, no base, no instance state. `Scheme` is
a small plain struct.

## 6. Public Member Variables

`Scheme`:

| Variable | Type | Description |
|----------|------|-------------|
| `name` | `QString` | Display name, from the scheme's `[General] Name` (or the file name). |
| `path` | `QString` | Absolute path to the `.colors` file. |

## 9. Public Methods

#### static QVector\<Scheme\> availableSchemes()

Scans `color-schemes` directories under the standard data locations
(`~/.local/share`, `/usr/share`, …) for `*.colors` files and returns them. The
list is empty on non-KDE systems.

#### static std::optional\<ThemeDefinition\> loadScheme(const QString &path)

Parses one `.colors` file into a theme. The application palette comes from the
`Colors:Window`, `Colors:View`, `Colors:Button`, `Colors:Selection` and
`Colors:Tooltip` groups; the markdown roles are derived from it; and the syntax
palette is taken from the built-in light or dark set according to the scheme's
lightness. Returns `std::nullopt` when the essential colour groups are missing.

#### static QString currentSchemePath()

Resolves the scheme the desktop is currently using, from the `[General]
ColorScheme` entry in `kdeglobals`, to the matching `.colors` file. Returns an
empty string when it cannot be determined.

#### static std::optional\<ThemeDefinition\> currentSystemTheme()

Convenience: the loaded theme for `currentSchemePath()`, or `std::nullopt`.

## 11. Ownership and Lifecycle

No ownership; all methods are pure and return values.

## 12. Thread Safety

Stateless and thread-safe (only Qt value types and filesystem reads).

## 14. Inter-Class Interactions

- `ThemeManager::resolve` prefers the current system theme in System mode.
- `MainWindow` lists `availableSchemes()` under View → Theme → System Schemes.

## 16. Usage Example

```cpp
if (auto theme = SystemThemeProvider::currentSystemTheme())
    QApplication::setPalette(theme->palette);
```