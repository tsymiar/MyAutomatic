import { webviewRender } from './view_html';

interface CommonFile {
  file: string;
  ratio: number;
}

interface AnalysisSummary {
  common_lines: number;
  total_lines: number;
  effective_lines: number;
  common_ratio: number;
}

export interface ComparisonFolderResults {
  source: string;
  target: string;
  source_unique: string[];
  target_unique: string[];
  different_files: Array<{
    detail: CommonFile;
    line: number;
    size: number;
    hash?: string;
  }>;
  matched_files: string[];
  common_files: Array<CommonFile>;
  analysis_summary?: AnalysisSummary;
}

function emptyResult(source: string, target: string): ComparisonFolderResults {
  return {
    source,
    target,
    source_unique: [],
    target_unique: [],
    different_files: [],
    matched_files: [],
    common_files: [],
    analysis_summary: {
      common_lines: 0,
      total_lines: 0,
      effective_lines: 0,
      common_ratio: 0
    }
  };
}

function num(x: any): number {
  return typeof x === 'number' && isFinite(x) ? x : 0;
}

/**
 * Map similary.py's `--json` payload (directory or single-file mode) onto the
 * structure the Webview expects. Ratios are 0-100 percentages.
 */
function parseJsonResult(obj: any, source: string, target: string): ComparisonFolderResults | null {
  if (!obj || typeof obj !== 'object') {
    return null;
  }

  const result = emptyResult(source, target);

  // Directory comparison: { total, details[], unmatched{source,target} }
  if (obj.total && Array.isArray(obj.details) && obj.unmatched) {
    const total = obj.total;
    result.analysis_summary = {
      common_lines: num(total.common),
      total_lines: num(total.orig1),
      effective_lines: Math.max(num(total.valid1), num(total.valid2)),
      common_ratio: num(total.ratio)
    };

    const um = obj.unmatched;
    result.source_unique = Array.isArray(um.source) ? um.source.map(String) : [];
    result.target_unique = Array.isArray(um.target) ? um.target.map(String) : [];

    for (const d of obj.details) {
      const pct = num(d.ratio);
      const fileName = typeof d.file_name === 'string' ? d.file_name : '';
      if (pct >= 100) {
        result.common_files.push({ file: fileName, ratio: pct });
      } else {
        result.different_files.push({
          detail: { file: fileName, ratio: pct },
          line: 0,
          size: 0,
          hash: ''
        });
      }
    }
    result.matched_files = result.common_files
      .map(f => f.file)
      .concat(result.different_files.map(f => f.detail.file));
    return result;
  }

  // Single-file comparison: a flat object carrying a `ratio` field.
  if (typeof obj.ratio === 'number') {
    const pct = num(obj.ratio);
    result.analysis_summary = {
      common_lines: num(obj.common),
      total_lines: num(obj.orig1),
      effective_lines: Math.max(num(obj.valid1), num(obj.valid2)),
      common_ratio: pct
    };
    const fileName = typeof obj.file_name === 'string' ? obj.file_name : '';
    if (pct >= 100) {
      result.common_files.push({ file: fileName, ratio: pct });
    } else {
      result.different_files.push({
        detail: { file: fileName, ratio: pct },
        line: 0,
        size: 0,
        hash: ''
      });
    }
    result.matched_files = result.common_files
      .map(f => f.file)
      .concat(result.different_files.map(f => f.detail.file));
    return result;
  }

  return null;
}

/**
 * Parse similary.py's `--json` stdout into the structure the Webview expects.
 */
export function parseComparisonOutput(
  text: string,
  source: string,
  target: string
): ComparisonFolderResults {
  const obj = JSON.parse(text.trim());
  const result = parseJsonResult(obj, source, target);
  if (!result) {
    throw new Error('Unexpected similary.py JSON payload');
  }
  return result;
}

export function getWebviewContent(result: ComparisonFolderResults): string {
  const maxDisplayLen = 80;
  const escapeHtml = (str: string) => {
    if (!str) return '';
    return str.replace(/&/g, '&amp;')
              .replace(/</g, '&lt;')
              .replace(/>/g, '&gt;')
              .replace(/"/g, '&quot;')
              .replace(/'/g, '&#39;');
  };
  const shorten = (s: string) => {
    if (!s) return '';
    if (s.length <= maxDisplayLen) return s;
    const headLen = Math.floor(maxDisplayLen / 2) - 2;
    const tailLen = Math.ceil(maxDisplayLen / 2) - 2;
    return s.substring(0, headLen) + '…' + s.substring(s.length - tailLen);
  };
  const ratioPercent = (r: number) => r.toFixed(1);
  const ratioColor = (r: number) => {
    if (r >= 100) { return '#10b981'; }
    if (r >= 70) { return '#22c55e'; }
    if (r >= 30) { return '#f59e0b'; }
    return '#ef4444';
  };

  const buildFileList = (files: string[]) => {
    if (!files.length) { return '<li class="empty">None</li>'; }
    return files.map(f => {
      const full = escapeHtml(f);
      return `<li title="${full}">${escapeHtml(shorten(f))}</li>`;
    }).join('');
  };

  const buildDiffRows = () => {
    if (!result.different_files.length) {
      return '<tr><td colspan="4" class="empty-row">No differences found</td></tr>';
    }
    return result.different_files.map(f => `
      <tr>
        <td class="file-cell" title="${escapeHtml(f.detail.file)}">${escapeHtml(shorten(f.detail.file))}</td>
        <td class="ratio-cell"><span class="ratio-bar" style="--ratio:${ratioPercent(f.detail.ratio)}%;--color:${ratioColor(f.detail.ratio)}"></span>${ratioPercent(f.detail.ratio)}%</td>
        <td class="size-cell">${formatSize(f.size)}</td>
        <td class="hash-cell">${escapeHtml(f.hash?.substring(0, 8) ?? '—')}</td>
      </tr>
    `).join('');
  };

  const buildAnalysis = () => {
    const s = result.analysis_summary;
    if (!s || (s.common_lines === 0 && s.total_lines === 0)) { return ''; }
    return `
<div class="section ana">
  <div class="section-header"><span class="dot"></span>Analysis Summary</div>
  <div class="analysis-grid">
    <div class="ana-item">
      <div class="ana-val">${s.common_lines}</div>
      <div class="ana-lbl">Duplicate Lines</div>
    </div>
    <div class="ana-item">
      <div class="ana-val">${s.total_lines}</div>
      <div class="ana-lbl">Total Original Lines</div>
    </div>
    <div class="ana-item">
      <div class="ana-val">${s.effective_lines}</div>
      <div class="ana-lbl">Effective Lines (Ratio Base)</div>
    </div>
    <div class="ana-item">
      <div class="ana-val">${s.common_ratio.toFixed(2)}%</div>
      <div class="ana-lbl">Overall Similarity</div>
    </div>
  </div>
</div>`;
  };

  return webviewRender({
    sourceShort: shorten(result.source),
    targetShort: shorten(result.target),
    sourceFull: escapeHtml(result.source),
    targetFull: escapeHtml(result.target),
    sourceUniqueCount: result.source_unique.length ?? 0,
    targetUniqueCount: result.target_unique.length ?? 0,
    commonCount: result.common_files.length ?? 0,
    differentCount: result.different_files.length ?? 0,
    sourceUniqueList: buildFileList(result.source_unique),
    targetUniqueList: buildFileList(result.target_unique),
    diffTableRows: buildDiffRows(),
    analysisHtml: buildAnalysis(),
  });
}

function formatSize(bytes: number): string {
  if (bytes === 0) { return '0 Bytes'; }
  const k = 1024;
  const sizes = ['Bytes', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}
