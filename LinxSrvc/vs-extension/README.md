# Files Comparison

A **VS Code extension** that compares **two folders** per file and reports
similarity in a Webview. Comments and blank lines are ignored, so renaming or
reformatting does not mask a copy. Requires **Python 3** on PATH (`python` on
Windows); extension id `similarity`.

## Install

Install the VSIX via **Extensions: Install from VSIX...** in VS Code, or from
a terminal:

```bash
code --install-extension similarity-*.vsix
```

## Usage

Right-click two folders in the Explorer and choose
**Compare Selected Directories**, or run **Quick Compare Selected Folders**
from the command palette. Results open in a Webview; the **Files Comparison**
output channel holds the raw log.

| command | VS Code entry point |
|---|---|
| `similarity.compareFolders` | Explorer folder context menu |
| `similarity.quickCompare` | palette + `Ctrl+Alt+Shift+C` (`Cmd` on macOS) |
| `similarity.openResults` | not shown in the command palette |

With fewer than two folders selected, a picker offers:

- **Reuse last comparison** — previous pair, remembered per workspace
- **Choose folders** — pick both in one dialog (source first); picking one
  folder opens a target picker with last-used and workspace suggestions

A **checkbox picker** then lists only the extensions found in the two
selected folders, with file counts, most frequent first. Previous checks
start out checked. The selection is saved to the
`similarity.ignoreExtensions` setting (comma-separated), which is also
available in the Settings UI. `Esc` cancels the comparison; confirming with
nothing checked compares all files. Extension matching is case-insensitive
and uses a leading dot.

## Report

- **Unmatched files** — source only / target only
- **Matched files** — similarity % per pair: 🟢 ≥ 70%, 🟡 30–70%,
  🔴 < 30%
- **Summary** — counts, overall similarity, duplicate line total

*Effective lines = original lines − comments − blank lines*.
