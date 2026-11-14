import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';
import {
    parseComparisonOutput, ComparisonFolderResults, getWebviewContent
} from './parse_text';

export const QUICK_COMPARE_COMMAND = 'similarity.quickCompare';

const LAST_SOURCE_KEY = 'similarity.lastSource';
const LAST_TARGET_KEY = 'similarity.lastTarget';

const SCAN_LIMIT_DIRS = 1000;
const SKIPPED_SCAN_DIRS = new Set([
    '.git', '.hg', '.svn', '.venv', 'venv', '__pycache__', 'node_modules',
    '.cache'
]);

interface FolderPair {
    source: string;
    target: string;
}

interface ExtensionItem extends vscode.QuickPickItem {
    ext: string;
}

/**
 * Palette and keybinding entry point. Explorer arguments are forwarded to the
 * main command so both entries share one comparison flow.
 */
export function registerQuickCompareShortcut(context: vscode.ExtensionContext) {
    const disposable = vscode.commands.registerCommand(
        QUICK_COMPARE_COMMAND,
        (...args: any[]) => vscode.commands.executeCommand(
            'similarity.compareFolders', ...args
        )
    );
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

    registerQuickCompareShortcut(context);

    context.subscriptions.push(disposableCompare, disposableOpenResults, outChannel);
}

async function handleCompareCommand(
    context: vscode.ExtensionContext,
    outChannel: vscode.OutputChannel,
    args: any[]
): Promise<void> {
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
            rememberPair(context, source, target);
            if (folderPaths.length > 2) {
                outChannel.appendLine(`Selected ${folderPaths.length} folders, comparing first two:`);
            }
            outChannel.appendLine(`Comparing below folder files:\n  ${source}\n  ${target}`);
        } else {
            // Zero or one folder from Explorer → interactive picker
            const pair = await chooseFoldersInteractively(context, folderPaths[0]);
            if (!pair) {
                outChannel.appendLine('Folder selection canceled!');
                return;
            }
            source = pair.source;
            target = pair.target;
            rememberPair(context, source, target);
            outChannel.appendLine(`Comparing below folder files:\n  ${source}\n  ${target}`);
        }

        const ignoreExt = await getIgnoreExtensions(source, target);
        if (ignoreExt === undefined) {
            outChannel.appendLine('Extension selection canceled!');
            return;
        }
        outChannel.appendLine(`Ignoring extensions: ${ignoreExt.join(', ') || 'none'}`);

        const result = await runFolderComparison(context, source, target, ignoreExt, outChannel);
        showComparisonResults(result, outChannel);
    } catch (error: any) {
        outChannel.appendLine(`Error: ${error?.message ?? String(error)}`);
        outChannel.show();
        vscode.window.showErrorMessage(`Files comparison failed: ${error?.message ?? String(error)}`);
    }
}

function dirExists(p: string | undefined): p is string {
    if (!p) { return false; }
    try {
        return fs.statSync(p).isDirectory();
    } catch {
        return false;
    }
}

function getLastPair(context: vscode.ExtensionContext): FolderPair | undefined {
    const source = context.workspaceState.get<string>(LAST_SOURCE_KEY);
    const target = context.workspaceState.get<string>(LAST_TARGET_KEY);
    if (dirExists(source) && dirExists(target)) {
        return { source, target };
    }
    return undefined;
}

function rememberPair(context: vscode.ExtensionContext, source: string, target: string): void {
    context.workspaceState.update(LAST_SOURCE_KEY, source);
    context.workspaceState.update(LAST_TARGET_KEY, target);
}

function workspaceFolderPaths(): string[] {
    return (vscode.workspace.workspaceFolders ?? [])
        .map(folder => folder.uri.fsPath)
        .filter(dirExists);
}

async function openFolderDialog(title: string, multiSelect: boolean): Promise<string[]> {
    const uris = await vscode.window.showOpenDialog({
        canSelectFiles: false,
        canSelectFolders: true,
        canSelectMany: multiSelect,
        openLabel: title
    });
    return (uris ?? []).map(uri => uri.fsPath);
}

/**
 * Interactive picker for the two comparison folders.
 * - If a folder is already known (Explorer selection) it becomes the source and
 *   only the target is asked for.
 * - Otherwise the user can reuse the last pair or pick both folders in a single
 *   dialog (source first, then target). Picking one folder falls back to the
 *   target picker with workspace/last-used suggestions.
 */
async function chooseFoldersInteractively(
    context: vscode.ExtensionContext,
    preselectedSource?: string
): Promise<FolderPair | undefined> {
    const last = getLastPair(context);

    if (dirExists(preselectedSource)) {
        const target = await pickTarget(preselectedSource, last?.target);
        if (!target) { return undefined; }
        return { source: preselectedSource, target };
    }

    if (last) {
        const reuse = await vscode.window.showQuickPick(
            [
                {
                    label: '$(history) Reuse last comparison',
                    description: `${last.source}  ⇄  ${last.target}`,
                    pair: last as FolderPair | undefined
                },
                { label: '$(new-folder) Choose folders...', pair: undefined }
            ],
            { placeHolder: 'Compare two folders' }
        );
        if (!reuse) { return undefined; }
        if (reuse.pair) { return reuse.pair; }
    }

    // Pick both folders in one dialog; selection order defines source/target.
    const picked = await openFolderDialog(
        'Compare folders (pick 2: source first, then target)', true
    );
    if (picked.length === 0) { return undefined; }
    if (picked.length === 1) {
        const target = await pickTarget(picked[0], last?.target);
        if (!target) { return undefined; }
        return { source: picked[0], target };
    }
    return { source: picked[0], target: picked[1] };
}

/**
 * Target picker with convenient candidates: last-used target, workspace
 * folders, then a plain browse dialog.
 */
async function pickTarget(
    source: string,
    lastTarget?: string
): Promise<string | undefined> {
    interface TargetItem extends vscode.QuickPickItem {
        path?: string;
        browse?: boolean;
    }

    const items: TargetItem[] = [];
    if (dirExists(lastTarget) && lastTarget !== source) {
        items.push({
            label: '$(history) Last used target',
            description: lastTarget,
            path: lastTarget
        });
    }
    for (const ws of workspaceFolderPaths()) {
        if (ws !== source) {
            items.push({ label: '$(folder-active) Workspace folder', description: ws, path: ws });
        }
    }
    items.push({ label: '$(search) Browse for target folder...', browse: true });

    const picked = await vscode.window.showQuickPick(items, {
        placeHolder: `Select the target directory to compare against ${source}`
    });
    if (!picked) { return undefined; }

    if (picked.browse) {
        const browsed = await openFolderDialog('Select as target directory', false);
        return browsed[0];
    }
    return picked.path;
}

function normalizeExtensions(values: string[]): string[] {
    const result: string[] = [];
    for (const value of values) {
        let ext = value.trim().toLowerCase();
        if (ext.startsWith('*.')) {
            ext = ext.slice(1);
        }
        if (!ext.startsWith('.')) {
            ext = `.${ext}`;
        }
        if (!/^\.[a-z0-9_+~-]+$/.test(ext)) {
            continue;
        }
        if (!result.includes(ext)) {
            result.push(ext);
        }
    }
    return result;
}

/** Counts file extensions in both folders, bounded to keep the picker responsive. */
async function collectExtensions(
    folders: string[]
): Promise<Map<string, number>> {
    const counts = new Map<string, number>();
    const queue = [...folders];
    let scanned = 0;

    while (queue.length > 0 && scanned < SCAN_LIMIT_DIRS) {
        const dir = queue.shift()!;
        scanned++;
        let entries: fs.Dirent[];
        try {
            entries = await fs.promises.readdir(dir, { withFileTypes: true });
        } catch {
            continue;
        }
        for (const entry of entries) {
            if (entry.isDirectory()) {
                if (!SKIPPED_SCAN_DIRS.has(entry.name)) {
                    queue.push(path.join(dir, entry.name));
                }
                continue;
            }
            const ext = path.extname(entry.name).toLowerCase();
            if (ext) {
                counts.set(ext, (counts.get(ext) ?? 0) + 1);
            }
        }
    }
    return counts;
}

/**
 * Checkbox picker for the extensions to skip. Only extensions present in the
 * two selected folders are offered; the confirmed selection replaces the
 * saved setting. Returns undefined when the user dismisses the picker.
 */
async function getIgnoreExtensions(
    source: string,
    target: string
): Promise<string[] | undefined> {
    const config = vscode.workspace.getConfiguration('similarity');
    const configured = config.get<string>('ignoreExtensions', '');
    const previously = normalizeExtensions(configured.split(','));

    const counts = await collectExtensions([source, target]);
    const discovered = [...counts.keys()]
        .filter(ext => /^\.[a-z0-9_+~-]+$/.test(ext))
        .sort((a, b) => (counts.get(b)! - counts.get(a)!) || a.localeCompare(b));
    if (discovered.length === 0) {
        return [];
    }

    const items: ExtensionItem[] = discovered.map(ext => ({
        label: ext,
        description: `${counts.get(ext)} file(s)`,
        picked: previously.includes(ext),
        ext
    }));

    const picked = await vscode.window.showQuickPick(items, {
        canPickMany: true,
        title: 'Ignore file extensions',
        placeHolder: 'Check extensions to skip; leave all unchecked to compare everything'
    });
    if (picked === undefined) {
        return undefined;
    }

    const selected = picked.map(item => item.ext);
    try {
        const configTarget = vscode.workspace.workspaceFolders
            ? vscode.ConfigurationTarget.Workspace
            : vscode.ConfigurationTarget.Global;
        await config.update('ignoreExtensions', selected.join(', '), configTarget);
    } catch {
        // Comparison still runs even when settings.json cannot be written.
    }
    return selected;
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
async function getFoldersFromExplorerArgs(args?: any): Promise<string[]> {
    if (args === undefined || args === null) {
        return [];
    }

    // Explorer passes [Uri, Uri[]]; expand nested arrays into a flat list.
    const flattenOnce = (input: any): any[] => {
        if (!Array.isArray(input)) { return [input]; }
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

        const args = [
            scriptPath,
            `"${source}"`,
            `"${target}"`,
            "--detail",
            "--json",
            "--no-color",
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

            try {
                const result = parseComparisonOutput(output, source, target);
                outChannel.appendLine(`Parsed output: ${JSON.stringify(result, null, 2)}`);
                if (result && result.source && result.target) {
                    resolve(result);
                } else {
                    reject(new Error('Parsed comparison result is incomplete'));
                }
            } catch (e: any) {
                outChannel.appendLine(`Raw output from Python:\n${output}`);
                reject(new Error(`Failed to parse output: ${e?.message ?? e}`));
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
    // Open a webview with the detailed results
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
        `Common: ${result.common_files.length ?? 0}`,
        'Show Log'
    ).then(choice => {
        if (choice === 'Show Log') {
            outChannel.show();
        }
    });
}

export function deactivate() { }
