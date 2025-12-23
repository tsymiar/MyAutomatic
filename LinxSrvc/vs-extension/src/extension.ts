import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';
import {
    parseComparisonResult, ComparisonFolderResults
} from './parse_text';

export const QUICK_COMPARE_COMMAND = 'similarity.quickCompare';

/**
 * Register a lightweight command intended to be bound to a keyboard shortcut.
 * It simply delegates to the main 'similarity.compareFolders' command so the keybinding
 * can be declared in package.json without duplicating logic.
 */
export function registerQuickCompareShortcut(context: vscode.ExtensionContext) {
    const disposable = vscode.commands.registerCommand(QUICK_COMPARE_COMMAND, async (...args: any[]) => {
        // Create an output channel for any explorer-invoked flow (will be disposed with the extension)
        const outChannel = vscode.window.createOutputChannel('Files Comparison');
        context.subscriptions.push(outChannel);

        // First try to handle Explorer-style invocation (two selected folders) without prompts.
        try {
            const handled = await compareFromExplorerArgs(context, outChannel, args);
            if (!handled) {
                // Fallback to the full interactive compare command
                await vscode.commands.executeCommand('similarity.compareFolders', ...args);
            }
        } catch (err) {
            // Ensure fallback to main command on unexpected errors
            await vscode.commands.executeCommand('similarity.compareFolders', ...args);
        }
    });

    context.subscriptions.push(disposable);
}

export function activate(context: vscode.ExtensionContext) {
    const outChannel = vscode.window.createOutputChannel('Files Comparison');

    // 注册比较文件夹命令
    const disposableCompare = vscode.commands.registerCommand('similarity.compareFolders', async (...args: any[]) => {
        outChannel.show();
        outChannel.clear();
        outChannel.appendLine('Starting files comparison...');

        try {
            // 先检查 Python
            await checkPythonInstallation(outChannel);

            // 尝试从 Explorer 参数中取出已选文件夹（支持一次选中两个、或仅选中一个）
            const folders = await getFoldersFromExplorerArgs(args);
            let source: string | undefined;
            let target: string | undefined;

            if (folders.source && folders.target) {
                // 用户在 Explorer 中一次选中了两个文件夹：直接比较，跳过对话框
                source = folders.source;
                target = folders.target;
                outChannel.appendLine('Starting files comparison from Explorer selection...');
                outChannel.appendLine(`Comparing below folder files:\n  ${source}\n  ${target}`);
            } else if (folders.source && !folders.target) {
                // 用户在 Explorer 中右键调用且只选中了一个文件夹：用其作为第一个文件夹，提示选择第二个
                source = folders.source;
                outChannel.appendLine(`Using Explorer selected folder as first directory: ${source}`);
                target = await selectFolder('Select as target directory');
                if (!target) {
                    outChannel.appendLine('Second directory selection canceled!');
                    return;
                }
            } else {
                // 无 Explorer 参数：交互式选择两个文件夹
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

            const result = await runFolderComparison(
                context,
                source,
                target,
                ignoreExt,
                outChannel
            );

            showComparisonResults(result, outChannel);
        } catch (error: any) {
            outChannel.appendLine(`Error: ${error?.message ?? String(error)}`);
            vscode.window.showErrorMessage(`Files comparison failed: ${error?.message ?? String(error)}`);
        }
    });

    // 注册打开结果视图命令
    const disposableOpenResults = vscode.commands.registerCommand('similarity.openResults', () => {
        outChannel.show();
    });

    context.subscriptions.push(disposableCompare, disposableOpenResults, outChannel);
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
    return new Promise((resolve, reject) => {
        const pythonCommand = os.platform() === 'win32' ? 'python' : 'python3';

        cp.exec(`${pythonCommand} --version`, (error: any, stdout: any, stderr: any) => {
            if (error) {
                outChannel.appendLine('Python check failed. Trying python3...');
                cp.exec('python3 --version', (error3: any) => {
                    if (error3) {
                        outChannel.appendLine('Python not found. Please install Python and add it to PATH.');
                        reject(new Error('Python not found. Please install Python and add it to PATH.'));
                    } else {
                        outChannel.appendLine('Python 3 detected');
                        resolve();
                    }
                });
            } else {
                outChannel.appendLine(`Python detected: ${stdout || stderr}`);
                resolve();
            }
        });
    });
}
// Try to resolve two selected folders passed from Explorer and, if present, run comparison without prompts.
export async function getFoldersFromExplorerArgs(args?: any): Promise<{ source?: string; target?: string }> {
    // Normalize to a flat array of potential items
    let items: any[] = [];
    if (args === undefined || args === null) {
        return {};
    }
    if (Array.isArray(args)) {
        items = args.slice();
        // sometimes the first element is itself an array of URIs
        if (items.length === 1 && Array.isArray(items[0])) {
            items = items[0].slice();
        }
    } else {
        items = [args];
    }

    const candidatePaths: string[] = [];

    const extractPath = (it: any): string | undefined => {
        if (!it) return undefined;
        // vscode.Uri
        if (it instanceof vscode.Uri) return it.fsPath;
        // object with fsPath
        if (typeof it === 'object' && typeof it.fsPath === 'string') return it.fsPath;
        // object with resourceUri (Tree items)
        if (typeof it === 'object' && it.resourceUri instanceof vscode.Uri) return it.resourceUri.fsPath;
        if (typeof it === 'object' && typeof it.resourceUri === 'string') return it.resourceUri;
        // plain string path
        if (typeof it === 'string') return it;
        return undefined;
    };

    for (const it of items) {
        const p = extractPath(it);
        if (!p) continue;
        // avoid duplicates
        if (candidatePaths.includes(p)) continue;
        try {
            const stat = fs.statSync(p);
            if (stat.isDirectory()) {
                candidatePaths.push(p);
            }
        } catch {
            // ignore non-existing or inaccessible paths
        }
        if (candidatePaths.length >= 2) break;
    }

    if (candidatePaths.length >= 2) {
        return { source: candidatePaths[0], target: candidatePaths[1] };
    } else if (candidatePaths.length === 1) {
        return { source: candidatePaths[0] };
    }
    return {};
}

// Convenience function to be called from the command handler when args may be provided.
// Returns true if explorer-based comparison was started, false otherwise.
export async function compareFromExplorerArgs(
  context: vscode.ExtensionContext,
  outChannel: vscode.OutputChannel,
  args?: any
): Promise<boolean> {
  const folders = await getFoldersFromExplorerArgs(args);
  if (!folders.source || !folders.target) {
    return false;
  }

  outChannel.show();
  outChannel.clear();
  outChannel.appendLine('Starting files comparison from Explorer selection...');
  outChannel.appendLine(`Comparing below folder files:\n  ${folders.source}\n  ${folders.target}`);

  try {
    await checkPythonInstallation(outChannel);

    const ignoreExt = await getIgnoreExtensions();
    outChannel.appendLine(`Ignoring extensions: ${ignoreExt.join(', ') || 'none'}`);

    const result = await runFolderComparison(
      context,
      folders.source,
      folders.target,
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
                        // throw new Error('Output does not contain valid JSON');
                        let jsonOut = parseComparisonResult(output, source, target); // --- IGNORE ---
                        let commonFiles = '';
                        for (const file of jsonOut.common_files) {
                            commonFiles += `"${file}", `;
                        }
                        cleanOutput = `{
                            "source": "${jsonOut.source}",
                            "target": "${jsonOut.target}",
                            "source_unique": ${JSON.stringify(jsonOut.source_unique)},
                            "target_unique": ${JSON.stringify(jsonOut.target_unique)},
                            "different_files": ${JSON.stringify(jsonOut.different_files)},
                            "matched_files": ${JSON.stringify(jsonOut.matched_files)},
                            "common_files": [${commonFiles.slice(0, -2)}]
                        }`;
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
        `Comparison completed. Unique: ${result.source_unique.length ?? result.target_unique.length ?? 0}, ` +
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

    return `
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Files Comparison Results</title>
        <style>
            body { color: #000; font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; padding: 20px; }
            .summary { background-color: #f5f5f5; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
            .summary h2 { margin-top: 0; color: #000; }
            .summary-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; }
            .summary-item { text-align: center; padding: 10px; border-radius: 5px; color: #000; word-break: break-all; }
            .unique1 { background-color: #ffcccc; }
            .unique2 { background-color: #ccffcc; }
            .common { background-color: #ccccff; }
            .different { background-color: #ffffcc; }
            .details { margin-top: 20px; }
            .details h3 { margin-top: 0; color: #c0c000; }
            table { width: 100%; border-collapse: collapse; color: #000; }
            th, td { padding: 8px 12px; text-align: left; border-bottom: 1px solid #666; color: #444000ff; max-width: 0; }
            th { background-color: #f2f2f2; }
            tr:hover { background-color: #f5f5f5; }
            li { color: #666; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
        </style>
    </head>
    <body>
        <div class="summary">
            <h2>Comparison Summary</h2>
            <div class="summary-grid">
                <div class="summary-item unique1">
                    <div class="count">${result.source_unique.length ?? 0} unique</div>
                    <div><br><span title="${escapeHtml(result.source)}" class="path">${escapeHtml(shorten(result.source))}</span></div>
                </div>
                <div class="summary-item unique2">
                    <div class="count">${result.target_unique.length ?? 0} unique</div>
                    <div><br><span title="${escapeHtml(result.target)}" class="path">${escapeHtml(shorten(result.target))}</span></div>
                </div>
                <div class="summary-item common">
                    <div class="count">${result.common_files.length ?? 0}</div>
                    <div>Common Files</div>
                </div>
                <div class="summary-item different">
                    <div class="count">${result.different_files.length ?? 0}</div>
                    <div>Different Files</div>
                </div>
            </div>
        </div>
        <div class="details">
            <h3>Source Unique</h3>
            <ul>
                ${
                    result.source_unique.map(file => {
                        const full = escapeHtml(file);
                        const disp = escapeHtml(shorten(file));
                        return `<li title="${full}">${disp}</li>`;
                    }).join('') || '<li>None</li>'
                }
            </ul>
            <h3>Target Unique</h3>
            <ul>
                ${
                    result.target_unique.map(file => {
                        const full = escapeHtml(file);
                        const disp = escapeHtml(shorten(file));
                        return `<li title="${full}">${disp}</li>`;
                    }).join('') || '<li>None</li>'
                }
            </ul>
            <h3>Different Files</h3>
            <table>
                <thead>
                    <tr>
                        <th>File</th>
                        <th>Ratio(%)</th>
                        <th>Size</th>
                        <th>Hash</th>
                    </tr>
                </thead>
                <tbody>
                    ${result.different_files.map(file => `
                        <tr>
                            <td>${file.detail.file}</td>
                            <td>${file.detail.ratio}</td>
                            <td>${formatSize(file.size)}</td>
                            <td>${file.hash?.substring(0, 8)}...</td>
                        </tr>
                    `).join('') || '<tr><td colspan="5">No differences found</td></tr>'}
                </tbody>
            </table>
        </div>
        <script>
            // client-side safety: ensure long byte numbers are readable if any raw numbers pass through
            document.querySelectorAll('td:nth-child(2), td:nth-child(3)').forEach(td => {
                const txt = td.textContent || '';
                if (/^\d+$/.test(txt)) {
                    const bytes = parseInt(txt, 10);
                    if (!isNaN(bytes)) {
                        const k = 1024;
                        const sizes = ['Bytes', 'KB', 'MB', 'GB'];
                        const i = Math.floor(Math.log(bytes) / Math.log(k)) || 0;
                        td.textContent = (bytes === 0 ? '0 Bytes' : parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i]);
                    }
                }
            });
        </script>
    </body>
    </html>
    `;
}

function formatSize(bytes: number): string {
    if (bytes === 0) { return '0 Bytes'; }
    const k = 1024;
    const sizes = ['Bytes', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

export function deactivate() { }
