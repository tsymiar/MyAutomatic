
interface CommonFile {
  file: string;
  ratio: number;
}

interface ComparisonResult {
  source: string;
  target: string;
  unique_source_files: string[];
  unique_target_files: string[];
  different_files: string[];
  common_files: CommonFile[];
}

interface AnalysisSummary {
  unique_source_count: number;
  unique_target_count: number;
  different_count: number
  common_count: number;
  common_lines: number;
  total_lines: number;
  common_ratio: number;
}

export interface FolderComparisonResult {
  source: string;
  target: string;
  unique_source_files: string[];
  unique_target_files: string[];
  different_files: Array<{
		file: string;
		line: number;
		hash: string;
	}>;
  common_files: Array<CommonFile>;
  analysis_summary?: AnalysisSummary;
}

export
function parseComparisonResult(text: string, source: string, target: string): ComparisonResult {
  const result: ComparisonResult = {
    source: source,
    target: target,
    unique_source_files: [],
    unique_target_files: [],
    different_files: [],
    common_files: []
  };

  const lines = text.split('\n');
  let currentSection = '';

  for (let i = 0; i < lines.length; i++) {
    const rawLine = lines[i];
    const line = rawLine.trim();

    // accumulate original lines for an optimized JSON-friendly formatted output
    if (!(result as any).reportLines) {
      (result as any).reportLines = [];
    }
    (result as any).reportLines.push(rawLine);

    if (line.includes('=== 未匹配文件 ===')) {
      currentSection = 'unmatched';
    } else if (line.includes('=== 匹配文件详情 ===')) {
      currentSection = 'matched';
    } else if (line.includes('=== 统计摘要 ===')) {
      currentSection = 'summary';
    } else if (line.includes('=== 综合分析 ===')) {
      currentSection = 'analysis';
    } else if (line.includes('=== Comparison Results ===')) {
      currentSection = 'comparison';
    }
    
    switch (currentSection) {
      case 'unmatched': {
        const consumed = parseUnmatchedSection(line, result, lines, i);
        if (consumed && consumed > 0) {
          i += consumed;
        }
        break;
      }
      case 'matched':
        parseMatchedSection(line, result);
        break;
      case 'summary':
      case 'analysis':
      case 'comparison':
        // Currently not processing these sections
        break;
    }
  }

  return result;
}

function parseUnmatchedSection(line: string, result: ComparisonResult, lines: string[], index: number): number {
  let consumed = 0;

  if (line.includes('源目录独有文件')) {
    let j = 1;
    while (lines[index + j] && lines[index + j].includes('•')) {
      const fileName = lines[index + j].trim().replace('•', '').trim();
      result.unique_source_files.push(fileName);
      j++;
    }
    consumed = Math.max(consumed, j - 1);
  }
  
  if (line.includes('目标目录独有文件')) {
    let j = 1;
    while (lines[index + j] && lines[index + j].includes('•')) {
      const fileName = lines[index + j].trim().replace('•', '').trim();
      result.unique_target_files.push(fileName);
      j++;
    }
    consumed = Math.max(consumed, j - 1);
  }

  return consumed;
}

function parseMatchedSection(line: string, result: ComparisonResult): void {
  if (line.includes('↔') && line.includes('相似度:')) {
    const match = line.match(/▏\s*(.+?)\s*↔/);
    if (match && match[1]) {
      const fileName = match[1].trim();
      const similarityMatch = line.match(/相似度:\s*([\d.]+)%/);
      if (similarityMatch) {
        const similarity = parseFloat(similarityMatch[1]);
        
        if (similarity < 100) {
          result.different_files.push(fileName);
        } else {
          result.common_files.push({
            file: fileName,
            ratio: 100
          });
        }
      }
    }
  }
}
