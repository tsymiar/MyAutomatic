import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';
import {
    parseComparisonResult, ComparisonFolderResults
} from './parse_text';
import { webviewRender } from './view_html';

export const QUICK_COMPARE_COMMAND = 'similarity.quickCompare';

/**
 * Register a lightweight command intended to be bound to a keyboard shortcut.
 * It simply delegates to the main 'similarity.compareFolders' command so the keybinding
 * can be declared in package.json without duplicating logic.
 */
export function registerQuickCompareShortcut(context: vscode.ExtensionContext) {
    const disposable = vscode.commands.registerCommand(QUICK_COMPARE_COMMAND, async (...args: any[]) => {
        const outChannel = vscode.window.createOutputChannel('Files Comparison');
        context.subscriptions.push(outChannel);

        try {
            const handled = await compareFromExplorerArgs(context, outChannel, args);
            if (!handled) {
                await vscode.commands.executeCommand('similarity.compareFolders', ...args);
            }
        } catch {
            await vscode.commands.executeCommand('similarity.compareFolders', ...args);
        }
    });

    context.subscriptions.push(disposable);
}

export function activate(context: vscode.ExtensionContext) {
    const outChannel = vscode.window.createOutputChannel('Files Comparison');

    const disposableCompare = vscode.commands.registerCommand(
        'similarity.compareFolders',
        async (...args: any[]) => handleCompareCommand(context, outChannel, args)
    );

    const disposableOpenResults = vscode.commands.registerCommand('similarity.openResults', () => {
        outChannel.show();
    });

    context.subscriptions.push(disposableCompare, disposableOpenResults, outChannel);
}

/**
 * 统一的文件夹比较命令处理逻辑，供 compareFolders 和 quickCompare 共用。
 */
async function handleCompareCommand(
    context: vscode.ExtensionContext,
    outChannel: vscode.OutputChannel,
    args: any[]
): Promise<void> {
    outChannel.show();
    outChannel.clear();
    outChannel.appendLine('Starting files comparison...');

    try {
        await checkPythonInstallation(outChannel);

        const folderPaths = await getFoldersFromExplorerArgs(args);
        let source: string | undefined;
        let target: string | undefined;

        if (folderPaths.length >= 2) {
            // Two or more folders selected → compare the first two directly
            source = folderPaths[0];
            target = folderPaths[1];
            if (folderPaths.length > 2) {
                outChannel.appendLine(`Selected ${folderPaths.length} folders, comparing first two:`);
            }
            outChannel.appendLine(`Comparing below folder files:\n  ${source}\n  ${target}`);
        } else if (folderPaths.length === 1) {
            source = folderPaths[0];
            outChannel.appendLine(`Using Explorer selected folder as first directory: ${source}`);
            target = await selectFolder('Select as target directory');
            if (!target) {
                outChannel.appendLine('Second directory selection canceled!');
                return;
            }
        } else {
            source = await selectFolder('Select as source directory');
            if (!source) {
                outChannel.appendLine('First directory selection canceled!');
                return;
            }
            target = await selectFolder('Select as target directory');
            if (!target) {
                outChannel.appendLine('Second directory selection canceled!');
                return;
            }
        }

        const ignoreExt = await getIgnoreExtensions();
        outChannel.appendLine(`Ignoring extensions: ${ignoreExt.join(', ') || 'none'}`);

        const result = await runFolderComparison(context, source, target, ignoreExt, outChannel);
        showComparisonResults(result, outChannel);
    } catch (error: any) {
        outChannel.appendLine(`Error: ${error?.message ?? String(error)}`);
        vscode.window.showErrorMessage(`Files comparison failed: ${error?.message ?? String(error)}`);
    }
}

async function selectFolder(prompt: string): Promise<string | undefined> {
    const uris = await vscode.window.showOpenDialog({
        canSelectFiles: false,
        canSelectFolders: true,
        canSelectMany: false,
        openLabel: prompt
    });

    return uris && uris.length > 0 ? uris[0].fsPath : undefined;
}

async function getIgnoreExtensions(): Promise<string[]> {
    const ignore = await vscode.window.showInputBox({
        prompt: 'Enter file extensions to ignore (comma-separated)',
        placeHolder: '.so, .gz, .log, .exe, .dll, .wps, .pdf, .jpg, .jpeg, .vsix',
        value: vscode.workspace.getConfiguration('similarity').get('ignoreExtensions', '')
    });

    if (!ignore) { return []; }

    return ignore.split(',')
        .map((ext: string) => ext.trim())
        .filter((ext: string) => ext !== '')
        .map((ext: string) => ext.startsWith('.') ? ext : `.${ext}`);
}

async function checkPythonInstallation(outChannel: vscode.OutputChannel): Promise<void> {
    const candidates = os.platform() === 'win32'
        ? ['python', 'python3']
        : ['python3', 'python'];

    for (const cmd of candidates) {
        try {
            const result = await new Promise<string>((resolve, reject) => {
                cp.exec(`${cmd} --version`, (error, stdout, stderr) => {
                    if (error) { reject(error); } else { resolve(stdout || stderr); }
                });
            });
            outChannel.appendLine(`Python detected: ${result.trim()}`);
            return;
        } catch {
            outChannel.appendLine(`${cmd} not found, trying next...`);
        }
    }
    throw new Error('Python not found. Please install Python and add it to PATH.');
}
// Try to resolve selected folders from Explorer context-menu args.
// VSCode passes (uri, selection[]) for explorer/context commands.
// Returns an array of valid directory paths (deduplicated, in selection order).
export async function getFoldersFromExplorerArgs(args?: any): Promise<string[]> {
    if (args === undefined || args === null) {
        return [];
    }

    // Flatten all args into a flat list, expanding nested arrays one level
    const flattenOnce = (input: any): any[] => {
        if (!Array.isArray(input)) { return [input]; }
        // If input looks like [Uri, Uri[]], flatten the second element too
        const result: any[] = [];
        for (const item of input) {
            if (Array.isArray(item)) {
                result.push(...item);
            } else {
                result.push(item);
            }
        }
        return result;
    };

    const items = flattenOnce(args);

    const extractPath = (it: any): string | undefined => {
        if (!it) return undefined;
        // vscode.Uri — use duck-typing instead of instanceof (module isolation safe)
        if (typeof it === 'object' && typeof it.fsPath === 'string' && typeof it.scheme === 'string') {
            return it.fsPath;
        }
        // object with resourceUri (Tree items)
        if (typeof it === 'object' && it.resourceUri) {
            if (typeof it.resourceUri.fsPath === 'string') { return it.resourceUri.fsPath; }
            if (typeof it.resourceUri === 'string') { return it.resourceUri; }
        }
        // plain string path
        if (typeof it === 'string') { return it; }
        return undefined;
    };

    const candidatePaths: string[] = [];

    for (const it of items) {
        const p = extractPath(it);
        if (!p) continue;
        if (candidatePaths.includes(p)) continue;
        try {
            const stat = fs.statSync(p);
            if (stat.isDirectory()) {
                candidatePaths.push(p);
            }
        } catch {
            // ignore non-existing or inaccessible paths
        }
    }

    return candidatePaths;
}

// Convenience function to be called from the command handler when args may be provided.
// Returns true if explorer-based comparison was started, false otherwise.
export async function compareFromExplorerArgs(
  context: vscode.ExtensionContext,
  outChannel: vscode.OutputChannel,
  args?: any
): Promise<boolean> {
  const folderPaths = await getFoldersFromExplorerArgs(args);
  if (folderPaths.length < 2) {
    return false;
  }

  const source = folderPaths[0];
  const target = folderPaths[1];

  outChannel.show();
  outChannel.clear();
  if (folderPaths.length > 2) {
    outChannel.appendLine(`Selected ${folderPaths.length} folders, comparing first two:`);
  }
  outChannel.appendLine(`Comparing below folder files:\n  ${source}\n  ${target}`);

  try {
    await checkPythonInstallation(outChannel);

    const ignoreExt = await getIgnoreExtensions();
    outChannel.appendLine(`Ignoring extensions: ${ignoreExt.join(', ') || 'none'}`);

    const result = await runFolderComparison(
      context,
      source,
      target,
      ignoreExt,
      outChannel
    );

    showComparisonResults(result, outChannel);
    return true;
  } catch (error: any) {
    outChannel.appendLine(`Error: ${error.message}`);
    vscode.window.showErrorMessage(`Files comparison failed: ${error.message}`);
    return true; // we handled the invocation (even on error)
  }
}
async function runFolderComparison(
    context: vscode.ExtensionContext,
    source: string,
    target: string,
    ignoreExt: string[],
    outChannel: vscode.OutputChannel
): Promise<ComparisonFolderResults> {
    return new Promise((resolve, reject) => {
        const pythonCommand = os.platform() === 'win32' ? 'python' : 'python3';
        const scriptPath = path.join(context.extensionPath, 'scripts', 'similarity.py');

        if (!fs.existsSync(scriptPath)) {
            reject(new Error('Python script not found'));
            return;
        }

        // 构建参数
        const args = [
            scriptPath,
            `"${source}"`,
            `"${target}"`,
            "--detail",
            ...ignoreExt.map(ext => `--ignore-ext=${ext}`)
        ];

        outChannel.appendLine(`Executing: ${pythonCommand} ${args.join(' ')}`);

        const startTime = Date.now();
        const pythonProcess = cp.spawn(pythonCommand, args, {
            shell: true,
            stdio: ['ignore', 'pipe', 'pipe']
        });

        let output = '';
        let errorOutput = '';

        pythonProcess.stdout.on('data', (data: any) => {
            const dataStr = data.toString();
            output += dataStr;
            outChannel.append(`\n ${dataStr}`);
        });

        pythonProcess.stderr.on('data', (data: any) => {
            const dataStr = data.toString();
            errorOutput += dataStr;
            outChannel.append(`[STDERR] ${dataStr}`);
        });

        pythonProcess.on('close', (code: number) => {
            const duration = (Date.now() - startTime) / 1000;
            outChannel.appendLine(`\nProcess completed in ${duration.toFixed(2)} seconds with exit code ${code}`);

            if (code !== 0) {
                reject(new Error(`Python process failed with code ${code}: ${errorOutput}`));
                return;
            }

            let outJson = null;
            try {
                // 清理 JSON 字符串
                let cleanOutput = output.replace(/[\x00-\x1F]/g, '');
                cleanOutput = cleanOutput.trim();

                // 验证 JSON 格式
                if (!cleanOutput.startsWith('{') || !cleanOutput.endsWith('}')) {
                    // 尝试提取有效的 JSON 部分
                    const jsonStart = cleanOutput.indexOf('{');
                    const jsonEnd = cleanOutput.lastIndexOf('}') + 1;

                    if (jsonStart !== -1 && jsonEnd > jsonStart) {
                        cleanOutput = cleanOutput.substring(jsonStart, jsonEnd);
                        outChannel.appendLine(`Extracted JSON from output: ${cleanOutput.substring(0, 100)}...`);
                    } else {
                        const jsonOut = parseComparisonResult(output, source, target);
                        cleanOutput = JSON.stringify(jsonOut, null, 2);
                    }
                }
                // 尝试解析 JSON
                outJson = cleanOutput;
                const result = JSON.parse(cleanOutput) as ComparisonFolderResults;
                outChannel.appendLine(`Parsed JSON output: ${JSON.stringify(result, null, 2)}`);
                if (result && result.source && result.target) {
                    resolve(result);
                }
            } catch (e) {
                outChannel.appendLine(`Raw output from Python:\n${output}`);
                reject(new Error(`Failed to parse JSON output: ${e}\nCleaned output: ${outJson}`));
            }
        });

        pythonProcess.on('error', (err: any) => {
            reject(new Error(`Failed to start Python process: ${err.message}`));
        });
    });
}

function showComparisonResults(result: ComparisonFolderResults, outChannel: vscode.OutputChannel) {
    outChannel.appendLine('\n=== Comparison Results ===');
    outChannel.appendLine(`Source unique: ${result.source_unique.length ?? 0}`);
    outChannel.appendLine(`Target unique: ${result.target_unique.length ?? 0}`);
    outChannel.appendLine(`Differences: ${result.different_files.length ?? 0}`);
    outChannel.appendLine(`Matched files: ${result.matched_files.length ?? 0}`);
    outChannel.appendLine(`Common files: ${result.common_files.length ?? 0}`);
    // 创建Web视图显示详细结果
    const panel = vscode.window.createWebviewPanel(
        'folderComparisonResults',
        'Folder Comparison Results',
        vscode.ViewColumn.Two,
        {}
    );

    panel.webview.html = getWebviewContent(result);

    vscode.window.showInformationMessage(
        `Comparison completed. Source unique: ${result.source_unique.length ?? 0}, ` +
        `Target unique: ${result.target_unique.length ?? 0}, ` +
        `Matched: ${result.matched_files.length ?? 0}, ` +
        `Different: ${result.different_files.length ?? 0}, ` +
        `Common: ${result.common_files.length ?? 0}`
    );
}

function getWebviewContent(result: ComparisonFolderResults): string {
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
    const ratioPercent = (r: number) => {
        if (r <= 1) { return (r * 100).toFixed(1); }
        return r.toFixed(1);
    };
    const ratioColor = (r: number) => {
        const pct = r <= 1 ? r * 100 : r;
        if (pct >= 100) { return '#10b981'; }
        if (pct >= 70) { return '#22c55e'; }
        if (pct >= 30) { return '#f59e0b'; }
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

export function deactivate() { }
