/**
 * Webview HTML template for the folder comparison results panel.
 *
 * Uses a tagged template literal function that receives pre-escaped HTML
 * fragments so the main extension.ts doesn't embed raw CSS/HTML.
 */

export interface WebviewData {
    // pre-escaped strings
    sourceShort: string;
    targetShort: string;
    sourceFull: string;
    targetFull: string;
    sourceUniqueCount: number;
    targetUniqueCount: number;
    commonCount: number;
    differentCount: number;
    sourceUniqueList: string;   // <li>...</li> items
    targetUniqueList: string;
    diffTableRows: string;       // <tr>...</tr> rows
    analysisHtml: string;        // full analysis section or empty string
}

export function webviewRender(data: WebviewData): string {
    return `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Files Comparison Results</title>
<style>
:root {
    --bg: #0f1117;
    --card: #161822;
    --card-hover: #1c1f2e;
    --border: #2a2d3e;
    --text: #e1e4ed;
    --text2: #8b90a0;
    --text3: #5c6070;
    --accent: #6366f1;
    --accent2: #818cf8;
    --red: #f43f5e;
    --green: #10b981;
    --blue: #3b82f6;
    --amber: #f59e0b;
    --purple: #8b5cf6;
    --radius: 8px;
    --font: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
    --mono: 'JetBrains Mono', 'Cascadia Code', 'Fira Code', Consolas, monospace;
}
* { margin: 0; padding: 0; box-sizing: border-box; }
body {
    font-family: var(--font);
    background: var(--bg);
    color: var(--text);
    padding: 20px 24px;
    line-height: 1.55;
}
.header {
    display: flex; align-items: center; gap: 12px;
    margin-bottom: 20px; padding-bottom: 14px;
    border-bottom: 1px solid var(--border);
}
.header-icon {
    width: 32px; height: 32px; border-radius: 8px;
    background: linear-gradient(135deg, var(--accent), #a855f7);
    display: flex; align-items: center; justify-content: center;
    font-size: 15px; flex-shrink: 0;
}
.header h1 { font-size: 16px; font-weight: 700; }
.header .sub {
    font-size: 11px; color: var(--text3);
    font-family: var(--mono); margin-top: 2px;
}

/* ---- stat cards (compact) ---- */
.stats {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 10px;
    margin-bottom: 20px;
}
@media (max-width: 640px) { .stats { grid-template-columns: repeat(2, 1fr); } }
.stat {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: 6px;
    padding: 10px 12px;
    text-align: center;
    transition: background .18s;
}
.stat:hover { background: var(--card-hover); }
.stat .val {
    font-size: 20px; font-weight: 700;
    margin-bottom: 2px; line-height: 1.2;
}
.stat .lbl {
    font-size: 10px; color: var(--text3);
    text-transform: uppercase; letter-spacing: .5px;
}
.stat.src .val { color: var(--red); }
.stat.tgt .val { color: var(--green); }
.stat.cmn .val { color: var(--blue); }
.stat.dif .val { color: var(--amber); }

/* ---- sections ---- */
.section {
    background: var(--card);
    border: 1px solid var(--border);
    border-radius: var(--radius);
    padding: 12px 16px;
    margin-bottom: 12px;
}
.section-header {
    display: flex; align-items: center; gap: 8px;
    margin-bottom: 10px;
    font-size: 11px; font-weight: 600; text-transform: uppercase;
    letter-spacing: .5px;
}
.section-header .dot {
    width: 7px; height: 7px; border-radius: 50%; flex-shrink: 0;
}
.section.src .section-header { color: var(--red); }  .section.src .dot { background: var(--red); }
.section.tgt .section-header { color: var(--green); } .section.tgt .dot { background: var(--green); }
.section.dif .section-header { color: var(--amber); } .section.dif .dot { background: var(--amber); }
.section.ana .section-header { color: var(--purple); } .section.ana .dot { background: var(--purple); }

/* ---- file list ---- */
.file-list { list-style: none; display: flex; flex-direction: column; gap: 2px; }
.file-list li {
    padding: 4px 8px; border-radius: 4px;
    font-size: 11px; font-family: var(--mono);
    color: var(--text2);
    white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    cursor: default;
    transition: background .12s;
}
.file-list li:hover { background: rgba(255,255,255,.04); color: var(--text); }
.file-list li.empty { color: var(--text3); font-style: italic; font-family: var(--font); }

/* ---- diff table ---- */
.table-wrap { overflow-x: auto; }
table { width: 100%; border-collapse: collapse; font-size: 11px; }
thead th {
    padding: 6px 10px; text-align: left; font-weight: 600;
    font-size: 10px; text-transform: uppercase; letter-spacing: .5px;
    color: var(--text3); border-bottom: 1px solid var(--border);
}
tbody td {
    padding: 6px 10px; border-bottom: 1px solid rgba(42,45,62,.5);
    color: var(--text2); font-family: var(--mono); font-size: 11px;
}
tbody tr:hover td { background: rgba(255,255,255,.02); color: var(--text); }
.file-cell { max-width: 300px; overflow: hidden; text-overflow: ellipsis; white-space: nowrap; }
.ratio-cell {
    display: flex; align-items: center; gap: 6px; white-space: nowrap;
}
.ratio-bar {
    display: inline-block; width: 44px; height: 4px; border-radius: 2px;
    background: rgba(255,255,255,.06);
    position: relative; flex-shrink: 0;
}
.ratio-bar::after {
    content: ''; position: absolute; left: 0; top: 0; height: 100%;
    border-radius: 2px;
    width: var(--ratio);
    background: var(--color);
}
.size-cell { white-space: nowrap; color: var(--text3); }
.hash-cell { font-size: 10px; color: var(--text3); }
.empty-row { text-align: center; color: var(--text3); padding: 20px 12px !important; }

/* ---- analysis summary ---- */
.analysis-grid {
    display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px;
}
@media (max-width: 500px) { .analysis-grid { grid-template-columns: 1fr; } }
.ana-item {
    text-align: center; padding: 10px 8px;
    background: rgba(255,255,255,.02); border-radius: 6px;
}
.ana-item .ana-val {
    font-size: 18px; font-weight: 700; color: var(--accent2);
}
.ana-item .ana-lbl {
    font-size: 10px; color: var(--text3); text-transform: uppercase;
    letter-spacing: .5px; margin-top: 2px;
}
</style>
</head>
<body>
<div class="header">
    <div class="header-icon">📊</div>
    <div>
        <h1>Folder Comparison Results</h1>
        <div class="sub" title="${data.sourceFull} ↔ ${data.targetFull}">${data.sourceShort} &nbsp;↔&nbsp; ${data.targetShort}</div>
    </div>
</div>

<div class="stats">
    <div class="stat src">
        <div class="val">${data.sourceUniqueCount}</div>
        <div class="lbl">Source Unique</div>
    </div>
    <div class="stat tgt">
        <div class="val">${data.targetUniqueCount}</div>
        <div class="lbl">Target Unique</div>
    </div>
    <div class="stat cmn">
        <div class="val">${data.commonCount}</div>
        <div class="lbl">Identical</div>
    </div>
    <div class="stat dif">
        <div class="val">${data.differentCount}</div>
        <div class="lbl">Different</div>
    </div>
</div>

<div class="section src">
    <div class="section-header"><span class="dot"></span>Source Unique Files (${data.sourceUniqueCount})</div>
    <ul class="file-list">${data.sourceUniqueList}</ul>
</div>

<div class="section tgt">
    <div class="section-header"><span class="dot"></span>Target Unique Files (${data.targetUniqueCount})</div>
    <ul class="file-list">${data.targetUniqueList}</ul>
</div>

<div class="section dif">
    <div class="section-header"><span class="dot"></span>Different Files (${data.differentCount})</div>
    <div class="table-wrap">
    <table>
        <thead><tr><th>File</th><th>Similarity</th><th>Size</th><th>Hash</th></tr></thead>
        <tbody>${data.diffTableRows}</tbody>
    </table>
    </div>
</div>
${data.analysisHtml}
</body>
</html>`;
}
