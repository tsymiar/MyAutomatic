# Files Comparison

A VS Code extension that compares **two folders** and reports per-file similarity.
Comments and whitespace are ignored so renaming or reformatting doesn't mask a copy;
results are rendered in a Webview, plus a JSON view for tooling.

- extension id `similarity`, version `1.0.1`
- TypeScript sources in `src/`, the actual analysis is done by a Python script
- requires **Python 3** available as `python3` (`python` on Windows)

## Install / Build

```bash
cd LinxSrvc/vs-extension
./make.sh                       # nvm + vsce → ../gen/similarity-1.0.1.vsix
code --install-extension ../gen/similarity-*.vsix
```

## Usage

Right-click two folders in the Explorer and pick **Compare Selected Directories**,
then read the results panel. Use **Files Comparison: Open Files Comparison Results**
to reopen the last report.

| command | where it shows up |
|---|---|
| `similarity.compareFolders` | Explorer context menu on folders (`explorerResourceIsFolder`) |
| `similarity.openResults` | registered, currently hidden from the palette (`when: false`) |
| `similarity.quickCompare` | declared in `package.json`; appears in the command palette and can be bound to a shortcut (falls back to the same flow as `compareFolders`) |

When prompted, you may enter comma-separated **file extensions to ignore**
(persisted in the `similarity.ignoreExtensions` setting).

## How it works

```text
src/extension.ts      picks the two folders + ignore extensions, spawns Python
   └─ python3 scripts/similarity.py "<source>" "<target>" --detail [--ignore-ext=…]
        └─ src/parse_text.ts   parses JSON output (falls back to plain text)
             └─ src/view_html.ts  renders the Webview report
```

`scripts/similarity.py` is a **symlink** to `../../../toolset/similary.py` — the real analyser
lives outside this folder. If that path is missing the comparison fails immediately with
*"Python script not found"*.

### Backend options (`toolset/similary.py`)

| flag | meaning |
|---|---|
| `source` / `target` | two positional paths, file **or** directory |
| `--diff` | print the line-level diff details |
| `--detail` | per-file detail in the report (always passed by the extension) |
| `--ignore-dirs a,b` | comma-separated directory names to skip |
| `--ignore-files *.tmp,*.bak` | comma-separated file patterns to skip |
| `--ignore-ext .log,.tmp` | extensions to skip; repeatable (`--ignore-ext=.log --ignore-ext=.tmp`), `.` prefix optional |
| `--no-color` | disable ANSI colors |
| `--json` | emit JSON instead of the human-readable report |

Ignored extensions are normalised to lowercase with a leading dot, so `.LOG`, `log` and `*.log`-style
input all work; matching happens in both directories before any file is read.

## Report sections

1. **Unmatched files** (source only / target only) with original and effective line counts
2. **Matched files** — similarity % per pair plus line statistics
   - 🟢 green ≥ 70% · 🟡 yellow 30–70% · 🔴 red < 30%
3. **Summary** — matched/unmatched counts, overall similarity, duplicate line total

*Effective lines = original lines − comments − blank lines*; similarity is computed on those only.
