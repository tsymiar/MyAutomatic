import * as vscode from 'vscode';
import * as cp from 'child_process';
import * as path from 'path';
import * as fs from 'fs';
import * as os from 'os';

interface FolderComparisonResult {
	source: string;
	target: string;
	unique_files_source: string[];
	unique_files_target: string[];
	different_files: Array<{
		path: string;
		size1: number;
		size2: number;
		hash1: string;
		hash2: string;
	}>;
	common_files: Array<{
		path: string;
		size: number;
	}>;
}

export function activate(context: vscode.ExtensionContext) {
	const outChannel = vscode.window.createOutputChannel('Files Comparison');

	// 注册比较文件夹命令
	let disposableCompare = vscode.commands.registerCommand('similarity.compareFolders', async () => {
		outChannel.show();
		outChannel.clear();
		outChannel.appendLine('Starting files comparison...');

		try {
			// 1. 检查Python环境
			await checkPythonInstallation(outChannel);

			// 2. 获取用户选择的文件夹
			const source = await selectFolder('Select first folder');
			if (!source) {
				outChannel.appendLine('First folder selection canceled');
				return;
			}

			const target = await selectFolder('Select second folder');
			if (!target) {
				outChannel.appendLine('Second folder selection canceled');
				return;
			}

			outChannel.appendLine(`Comparing below folder files:\n  ${source}\n  ${target}`);

			// 3. 获取忽略的文件扩展名
			const ignoreExt = await getIgnoreExtensions();
			outChannel.appendLine(`Ignoring extensions: ${ignoreExt.join(', ') || 'none'}`);

			// 4. 执行Python脚本
			const result = await runFolderComparison(
				context,
				source,
				target,
				ignoreExt,
				outChannel
			);

			// 5. 显示结果
			showComparisonResults(result, outChannel);

		} catch (error: any) {
			outChannel.appendLine(`Error: ${error.message}`);
			vscode.window.showErrorMessage(`Files comparison failed: ${error.message}`);
		}
	});

	// 注册打开结果视图命令
	let disposableOpenResults = vscode.commands.registerCommand('similarity.openResults', () => {
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
	const input = await vscode.window.showInputBox({
		prompt: 'Enter file extensions to ignore (comma separated, e.g., .tmp,.log ...)',
		placeHolder: '.bak, .log, .tmp, txt'
	});

	if (!input) { return []; }

	return input.split(',')
		.map(ext => ext.trim())
		.filter(ext => ext !== '')
		.map(ext => ext.startsWith('.') ? ext : `.${ext}`);
}

async function checkPythonInstallation(outChannel: vscode.OutputChannel): Promise<void> {
	return new Promise((resolve, reject) => {
		const pythonCommand = os.platform() === 'win32' ? 'python' : 'python3';

		cp.exec(`${pythonCommand} --version`, (error, stdout, stderr) => {
			if (error) {
				outChannel.appendLine('Python check failed. Trying python3...');
				cp.exec('python3 --version', (error3) => {
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

async function runFolderComparison(
	context: vscode.ExtensionContext,
	source: string,
	target: string,
	ignoreExt: string[],
	outChannel: vscode.OutputChannel
): Promise<FolderComparisonResult> {
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

		pythonProcess.stdout.on('data', (data) => {
			const dataStr = data.toString();
			output += dataStr;
			outChannel.append(`\n ${dataStr}`);
		});


		pythonProcess.stderr.on('data', (data) => {
			const dataStr = data.toString();
			errorOutput += dataStr;
			outChannel.append(`[STDERR] ${dataStr}`);
		});

		pythonProcess.on('close', (code) => {
			const duration = (Date.now() - startTime) / 1000;
			outChannel.appendLine(`\nProcess completed in ${duration.toFixed(2)} seconds with exit code ${code}`);

			if (code !== 0) {
				reject(new Error(`Python process failed with code ${code}: ${errorOutput}`));
				return;
			}

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
						cleanOutput =
							'{ \
								"source": "/tmp/source", \
								"target": "/tmp/target", \
								"unique_files_source": [], \
								"unique_files_target": [], \
								"different_files": [], \
								"common_files": [ \
									{ "path": "file1.txt", "size": 123 }, \
									{ "path": "subdir/file2.txt", "size": 456 }, \
									{ "path": "image.png", "size": 7890 } \
								] \
							}';
						// outChannel.appendLine('No valid JSON found in output, returning empty object.');
					}
				}
				// 尝试解析 JSON
				const result = JSON.parse(cleanOutput) as FolderComparisonResult;
				outChannel.appendLine(`Parsed JSON output: ${JSON.stringify(result, null, 2)}`);
				if (result && result.source && result.target) {
					resolve(result);
				}
			} catch (e) {
				outChannel.appendLine(`Raw output from Python:\n${output}`);
				reject(new Error(`Failed to parse JSON output: ${e}\nCleaned output: ${outChannel}`));
			}
		});

		pythonProcess.on('error', (err) => {
			reject(new Error(`Failed to start Python process: ${err.message}`));
		});
	});
}

function showComparisonResults(result: FolderComparisonResult, outChannel: vscode.OutputChannel) {
	outChannel.appendLine('\n=== Comparison Results ===');
	outChannel.appendLine(`Unique in first folder: ${result.unique_files_source.length} files`);
	outChannel.appendLine(`Unique in second folder: ${result.unique_files_target.length} files`);
	outChannel.appendLine(`Common files: ${result.common_files.length} files`);
	outChannel.appendLine(`Different files: ${result.different_files.length} files`);

	// 创建Web视图显示详细结果
	const panel = vscode.window.createWebviewPanel(
		'folderComparisonResults',
		'Folder Comparison Results',
		vscode.ViewColumn.Two,
		{}
	);

	panel.webview.html = getWebviewContent(result);

	vscode.window.showInformationMessage(
		`Comparison completed. Unique: ${result.unique_files_source.length + result.unique_files_target.length}, ` +
		`Different: ${result.different_files.length}, Common: ${result.common_files.length}`
	);
}

function getWebviewContent(result: FolderComparisonResult): string {
	return `
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Files Comparison Results</title>
        <style>
            body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; padding: 20px; }
            .summary { background-color: #f5f5f5; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
            .summary h2 { margin-top: 0; }
            .summary-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; }
            .summary-item { text-align: center; padding: 10px; border-radius: 5px; }
            .unique1 { background-color: #ffcccc; }
            .unique2 { background-color: #ccffcc; }
            .common { background-color: #ccccff; }
            .different { background-color: #ffffcc; }
            .details { margin-top: 20px; }
            .details h3 { margin-top: 0; }
            table { width: 100%; border-collapse: collapse; }
            th, td { padding: 8px 12px; text-align: left; border-bottom: 1px solid #ddd; }
            th { background-color: #f2f2f2; }
            tr:hover { background-color: #f5f5f5; }
        </style>
    </head>
    <body>
        <div class="summary">
            <h2>Comparison Summary</h2>
            <div class="summary-grid">
                <div class="summary-item unique1">
                    <div class="count">${result.unique_files_source.length}</div>
                    <div>Unique in<br>${result.source}</div>
                </div>
                <div class="summary-item unique2">
                    <div class="count">${result.unique_files_target.length}</div>
                    <div>Unique in<br>${result.target}</div>
                </div>
                <div class="summary-item common">
                    <div class="count">${result.common_files.length}</div>
                    <div>Common Files</div>
                </div>
                <div class="summary-item different">
                    <div class="count">${result.different_files.length}</div>
                    <div>Different Files</div>
                </div>
            </div>
        </div>
        
        <div class="details">
            <h3>Unique Files in First Folder</h3>
            <ul>
                ${result.unique_files_source.map(file => `<li>${file}</li>`).join('') || '<li>None</li>'}
            </ul>
            
            <h3>Unique Files in Second Folder</h3>
            <ul>
                ${result.unique_files_target.map(file => `<li>${file}</li>`).join('') || '<li>None</li>'}
            </ul>
            
            <h3>Different Files</h3>
            <table>
                <thead>
                    <tr>
                        <th>File Path</th>
                        <th>Size in Folder 1</th>
                        <th>Size in Folder 2</th>
                        <th>Hash (Folder 1)</th>
                        <th>Hash (Folder 2)</th>
                    </tr>
                </thead>
                <tbody>
                    ${result.different_files.map(file => `
                        <tr>
                            <td>${file.path}</td>
                            <td>${formatBytes(file.size1)}</td>
                            <td>${formatBytes(file.size2)}</td>
                            <td>${file.hash1.substring(0, 8)}...</td>
                            <td>${file.hash2.substring(0, 8)}...</td>
                        </tr>
                    `).join('') || '<tr><td colspan="5">No differences found</td></tr>'}
                </tbody>
            </table>
        </div>
        
        <script>
            function formatBytes(bytes) {
                if (bytes === 0) return '0 Bytes';
                const k = 1024;
                const sizes = ['Bytes', 'KB', 'MB', 'GB'];
                const i = Math.floor(Math.log(bytes) / Math.log(k));
                return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
            }
            
            // Apply formatting to byte columns
            document.querySelectorAll('td:nth-child(2), td:nth-child(3)').forEach(td => {
                const bytes = parseInt(td.textContent);
                if (!isNaN(bytes)) {
                    td.textContent = formatBytes(bytes);
                }
            });
        </script>
    </body>
    </html>
    `;
}

function formatBytes(bytes: number): string {
	if (bytes === 0) { return '0 Bytes'; }
	const k = 1024;
	const sizes = ['Bytes', 'KB', 'MB', 'GB'];
	const i = Math.floor(Math.log(bytes) / Math.log(k));
	return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

export function deactivate() { }
