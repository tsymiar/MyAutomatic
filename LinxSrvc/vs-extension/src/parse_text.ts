interface CommonFile {
  file: string;
  ratio: number;
}

interface AnalysisSummary {
  common_lines: number;
  total_lines: number;
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

export function parseComparisonResult(text: string, source: string, target: string): ComparisonFolderResults {
  const result: ComparisonFolderResults = {
    source: source,
    target: target,
    source_unique: [],
    target_unique: [],
    different_files: [],
    matched_files: [],
    common_files: [],
    analysis_summary: {
      common_lines: 0,
      total_lines: 0,
      common_ratio: 0
    }
  };

  const lines = text.split('\n');
  let currentSection = '';

  for (let i = 0; i < lines.length; i++) {
    const rawLine = lines[i];
    const line = rawLine.trim();

    // Detect section headers
    if (line.includes('=== 未匹配文件 ===')) {
      currentSection = 'unmatched';
    } else if (line.includes('=== 匹配文件详情')) {
      currentSection = 'matched';
    } else if (line.includes('=== 统计摘要 ===')) {
      currentSection = 'summary';
    } else if (line.includes('=== 综合分析 ===')) {
      currentSection = 'analysis';
    } else if (line.includes('=== Comparison Results ===')) {
      currentSection = 'comparison';
    }

    switch (currentSection) {
      case 'unmatched':
        parseUnmatchedSection(line, result, lines, i);
        break;
      case 'matched':
        parseMatchedSection(line, result);
        break;
      case 'summary':
        break;
      case 'analysis':
        parseAnalysisSection(line, result);
        break;
      case 'comparison':
        break;
    }
  }

  return result;
}

function parseMatchedSection(line: string, result: ComparisonFolderResults): void {
  // Match pattern like "▏ CMakeLists.txt ↔ CMakeLists.txt 相似度: 1.1%"
  if (line.includes('↔') && line.includes('相似度:')) {
    // Extract the file name before the arrow
    const match = line.match(/▏\s*(.+?)\s*↔/);
    if (match && match[1]) {
      const fileName = match[1].trim();
      result.matched_files.push(fileName);
      
      // Extract similarity percentage
      const similarityMatch = line.match(/相似度:\s*([\d.]+)%/);
      if (similarityMatch) {
        const similarity = parseFloat(similarityMatch[1]);
        
        if (similarity === 100) {
          result.common_files.push({
            file: fileName,
            ratio: 100
          });
        } else {
          result.different_files.push({
            detail: {
              file: fileName,
              ratio: similarity
            },
            line: 0,
            size: 0,
            hash: ''
          });
        }
      }
    }
  }
}

function parseUnmatchedSection(line: string, result: ComparisonFolderResults, lines: string[], index: number): void {
  if (line.includes('源目录独有文件')) {
    let j = 1;
    while (index + j < lines.length && lines[index + j] != null && lines[index + j].includes('•')) {
      const fileName = lines[index + j].trim().replace('•', '').trim();
      if (fileName) {
        result.source_unique.push(fileName);
      }
      j++;
    }
  } else if (line.includes('目标目录独有文件')) {
    let j = 1;
    while (index + j < lines.length && lines[index + j] != null && lines[index + j].includes('•')) {
      const fileName = lines[index + j].trim().replace('•', '').trim();
      if (fileName) {
        result.target_unique.push(fileName);
      }
      j++;
    }
  }
}

function parseAnalysisSection(line: string, result: ComparisonFolderResults): void {
  // Match pattern like "▏ 重复总行数: 2"
  if (line.includes('重复总行数:')) {
    const match = line.match(/重复总行数:\s*(\d+)/);
    if (match && match[1]) {
      result.analysis_summary!.common_lines = parseInt(match[1], 10);
    }
  }
  
  // Match pattern like "▏ 总原始行数: 33 → 有效 30"
  else if (line.includes('总原始行数:')) {
    // Extract the first number (original total lines)
    const match = line.match(/总原始行数:\s*(\d+)/);
    if (match && match[1]) {
      // The total lines should be the effective lines, but we'll store the original
      // If we need effective lines, we'd need to parse the "有效" part
      result.analysis_summary!.total_lines = parseInt(match[1], 10);
    }
  }
  
  // Match pattern like "▏ 综合重复率: 1.12% (基于有效行数)"
  else if (line.includes('综合重复率:')) {
    const match = line.match(/综合重复率:\s*([\d.]+)%/);
    if (match && match[1]) {
      result.analysis_summary!.common_ratio = parseFloat(match[1]);
    }
  }
}
