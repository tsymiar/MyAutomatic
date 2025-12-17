#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import difflib
import argparse
import fnmatch
from colorama import Fore, init
from collections import defaultdict
import re
import json, builtins, atexit

Style = None
init(autoreset=True)

COMMENT_TYPES = {
    "line": [
        (r"#.*$", re.MULTILINE),  # Python, Perl, Ruby等
        (r"//.*$", re.MULTILINE),  # C/C++/Java/JavaScript等
    ],
    "block": [
        (r"(\'\'\'(.*?)\'\'\'|\"\"\"(.*?)\"\"\")", re.DOTALL),  # Python
        (r"/\*.*?\*/", re.DOTALL),  # C/C++/Java/JavaScript等
        (r"<!--.*?-->", re.DOTALL),  # HTML/XML
    ],
}
COMMENT_PATTERNS = {
    ext: (COMMENT_TYPES["line"] + COMMENT_TYPES["block"])
    for ext in [
        ".py", ".java", ".js", ".cpp", ".c", ".rs", ".swift", ".kt", ".ts",
        ".css", ".scss", ".less", ".html", ".xml",
    ]
}
COMMENT_PATTERNS.update({".txt": [], ".md": [], ".json": [], ".yml": [], ".yaml": []})

class EmptyStyle:
    def __getattr__(self, name):
        return ""

def is_text_file(file_path):
    # 只需初始化一次，避免每次调用都创建集合
    if not hasattr(is_text_file, "_text_exts"):
        is_text_file._text_exts = set(COMMENT_PATTERNS.keys()).union(
            {".txt", ".md", ".json", ".yml", ".yaml"}
        )
    text_exts = is_text_file._text_exts
    file_ext = os.path.splitext(file_path)[1].lower()
    return file_ext in text_exts, file_ext

def delete_initial_block_comment(source):
    """
    删除文件开头的块注释（如C/Java的/* ... */），支持字符串或行列表输入。
    """
    if isinstance(source, str):
        lines = source.splitlines()
    else:
        lines = list(source)

    block_start = -1
    block_end = -1
    in_block_comment = False

    for index, line in enumerate(lines):
        if not in_block_comment:
            if line.strip().startswith('/*'):
                block_start = index
                in_block_comment = True
        else:
            if '*/' in line:
                block_end = index
                break

    if block_start != -1 and block_end != -1:
        lines = lines[:block_start] + lines[block_end+1:]

    return lines if isinstance(source, list) else "\n".join(lines)

def deal_comments_by_state_machine(code, file_ext):
    """
    使用状态机精确移除注释，保留字符串内的注释符号，并跟踪原始行号。
    返回: (处理后的代码行列表, 对应的原始行号列表)
    """
    if file_ext in ['.txt', '.md', '.json', '.yml', '.yaml']:
        return code.splitlines()  # 非代码文件直接返回原始行
    lines = []
    if isinstance(code, str):
        lines = code.splitlines()
    else:
        lines = list(code)
    # 根据文件扩展名确定注释规则
    c_family_comment = {'line': ['//'], 'block': [('/*', '*/')]}
    comment_rules = {
        '.py': {'line': ['#'], 'block': []},
        '.java': c_family_comment,
        '.js': c_family_comment,
        '.c': c_family_comment,
        '.cpp': c_family_comment,
        '.rs': c_family_comment,
        '.swift': c_family_comment,
        '.kt': c_family_comment,
        '.ts': c_family_comment,
        '.css': {'line': [], 'block': [('/*', '*/')]},
        '.scss': c_family_comment,
        '.less': c_family_comment,
        '.html': {'line': [], 'block': [('<!--', '-->')]},
        '.xml': {'line': [], 'block': [('<!--', '-->')]},
    }
    rules = comment_rules.get(file_ext, {'line': [], 'block': []})
    line_comment = rules['line'][0] if rules['line'] else None
    block_comment = rules['block'][0] if rules['block'] else None

    state = 'NORMAL'  # 状态: NORMAL, BLOCK_COMMENT, LINE_COMMENT, STRING_DOUBLE, STRING_SINGLE
    current_block_end = None
    striped_lines = []      # 处理后的代码行
    original_line_nums = []  # 对应的原始行号
    current_line = []
    escape = False
    current_line_num = 0

    for line_idx, line in enumerate(lines):
        i = 0
        n = len(line)
        current_line_num = line_idx  # 记录当前原始行号

        # 标记当前行是否产生了有效输出
        line_has_output = False

        while i < n:
            char = line[i]
            next_chars = line[i:i+2]  # 用于检查多字符注释符号

            if state == 'NORMAL':
                # 检查块注释开始
                if block_comment and next_chars == block_comment[0]:
                    state = 'BLOCK_COMMENT'
                    current_block_end = block_comment[1]
                    i += len(block_comment[0])
                    continue
                # 检查行注释开始
                elif line_comment and line.startswith(line_comment, i):
                    state = 'LINE_COMMENT'
                    i += len(line_comment)
                    break  # 跳过行剩余部分
                # 处理字符串
                elif char == '"':
                    state = 'STRING_DOUBLE'
                    current_line.append(char)
                    line_has_output = True
                elif char == "'":
                    state = 'STRING_SINGLE'
                    current_line.append(char)
                    line_has_output = True
                # 普通字符
                else:
                    if not char.isspace() or current_line or line_has_output:
                        current_line.append(char)
                        line_has_output = True
                    if char == '\\':  # 处理转义字符
                        escape = True
                    else:
                        escape = False

            elif state == 'BLOCK_COMMENT':
                # 检查块注释结束
                if next_chars == current_block_end:
                    state = 'NORMAL'
                    i += len(current_block_end)
                    continue
                # 否则跳过注释内容

            elif state in ('STRING_DOUBLE', 'STRING_SINGLE'):
                current_line.append(char)
                line_has_output = True
                # 检查字符串结束（忽略转义后的引号）
                if not escape and (
                    (state == 'STRING_DOUBLE' and char == '"') or
                    (state == 'STRING_SINGLE' and char == "'")
                ):
                    state = 'NORMAL'
                # 更新转义状态
                escape = (char == '\\' and not escape)

            i += 1  # 移动到下一个字符

        # 行结束处理
        if state == 'LINE_COMMENT':
            state = 'NORMAL'  # 行注释结束
            if current_line:
                # 添加当前行输出并记录原始行号
                striped_lines.append(''.join(current_line))
                original_line_nums.append(current_line_num)
                current_line = []
        elif state in ('STRING_DOUBLE', 'STRING_SINGLE'):
            # 字符串跨行时保留换行符
            current_line.append('\n')
            # 不立即结束行，继续到下一行
        elif state == 'NORMAL' and current_line:
            # 添加当前行输出并记录原始行号
            striped_lines.append(''.join(current_line))
            original_line_nums.append(current_line_num)
            current_line = []
        # BLOCK_COMMENT 状态继续到下一行，不添加内容
        elif line_has_output and not current_line:
            # 处理特殊情况：整行都是空格但被标记为有输出
            striped_lines.append('')
            original_line_nums.append(current_line_num)

    # 处理文件末尾剩余内容
    if current_line:
        striped_lines.append(''.join(current_line))
        original_line_nums.append(current_line_num)

    return striped_lines, original_line_nums

def deal_comments_by_line_marker(file_ext, code):
    patterns = COMMENT_PATTERNS.get(file_ext, [])
    if isinstance(code, str):
        code_lines = code.splitlines()
    else:
        code_lines = list(code)

    code_lines = delete_initial_block_comment(code_lines)

    def get_string_ranges(line):
        string_patterns = [
            (r"'[^'\\]*(?:\\.[^'\\]*)*'", 0),
            (r'"[^"\\]*(?:\\.[^"\\]*)*"', 0)
        ]
        ranges = []
        for pattern, _ in string_patterns:
            for match in re.finditer(pattern, line):
                ranges.append((match.start(), match.end()))
        return sorted(ranges)

    def non_string_ranges(line, string_ranges):
        if not string_ranges:
            return [(0, len(line))]
        result = []
        prev_end = 0
        for start, end in string_ranges:
            if start > prev_end:
                result.append((prev_end, start))
            prev_end = max(prev_end, end)
        if prev_end < len(line):
            result.append((prev_end, len(line)))
        return result

    def is_comment_in_non_string(line, pattern, flags, non_str_ranges):
        for match in re.finditer(pattern, line, flags=flags):
            start_pos, end_pos = match.start(), match.end()
            for range_start, range_end in non_str_ranges:
                if range_start <= start_pos < end_pos <= range_end:
                    return True
        return False

    def mark_comment_lines(code_lines, patterns):
        line_marker = [True] * len(code_lines)
        for idx, line in enumerate(code_lines):
            string_ranges = get_string_ranges(line)
            non_str_ranges = non_string_ranges(line, string_ranges)
            for pattern, flags in patterns:
                if is_comment_in_non_string(line, pattern, flags, non_str_ranges):
                    line_marker[idx] = False
                    break
        return line_marker

    def deal_single_line_comments(code_lines, line_marker):
        single_line_comment_pattern = re.compile(r'^\s*//.*$', re.MULTILINE)
        for i, line in enumerate(code_lines):
            if single_line_comment_pattern.match(line):
                line_marker[i] = False

    def process_block_comments(code_lines, line_marker):
        processed = []
        in_block_comment = False
        for _, (line, keep) in enumerate(zip(code_lines, line_marker)):
            if in_block_comment:
                if '*/' in line:
                    end_pos = line.find('*/') + 2
                    line = line[end_pos:].strip()
                    in_block_comment = False
                else:
                    continue
            if '/*' in line and '*/' not in line:
                start_pos = line.find('/*')
                line = line[:start_pos].strip()
                in_block_comment = True
            if '/*' in line and '*/' in line:
                start_pos = line.find('/*')
                end_pos = line.find('*/') + 2
                line = (line[:start_pos] + line[end_pos:]).strip()
            if keep and line.strip():
                processed.append(line.rstrip())
        return processed

    line_marker = mark_comment_lines(code_lines, patterns)
    deal_single_line_comments(code_lines, line_marker)
    return process_block_comments(code_lines, line_marker)

def strip_comments(file_ext, code, use_state_machine):
    if not use_state_machine:
        striped_lines = deal_comments_by_line_marker(file_ext, code)
    else:
        striped_lines, original_lines = deal_comments_by_state_machine(code, file_ext)
        original_lines = [str(i + 1) for i in original_lines]
        print([f"{line}  # Original line: {orig}" for line, orig in zip(striped_lines, original_lines)])
    return striped_lines

def parse_file(file_path, use_state_machine, cache=None):
    if cache is None:
        cache = {}
    if file_path in cache:
        return cache[file_path]
    try:
        is_text, file_ext = is_text_file(file_path)
        if not is_text:
            cache[file_path] = ([], 0)
            return [], 0
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
        original_lines = content.count("\n") + 1
        lines = (strip_comments(file_ext, content, use_state_machine), original_lines)
        cache[file_path] = lines
        return lines
    except Exception as e:
        print(f"{Fore.RED}文件读取错误 {os.path.basename(file_path)}: {str(e)}")
        cache[file_path] = ([], 0)
        return [], 0

def build_index(directory, ignore_dirs, ignore_files):
    index = defaultdict(list)
    ignore_dirs = set(ignore_dirs)
    ignore_files = set(ignore_files)
    for root, dirs, files in os.walk(directory, True):
        dirs[:] = [d for d in dirs if d not in ignore_dirs]
        for f in files:
            is_text, _ = is_text_file(f)
            if not is_text:
                continue
            if any(fnmatch.fnmatch(f, pattern) for pattern in ignore_files):
                continue
            path = os.path.join(root, f)
            rel_path = os.path.relpath(path, directory)
            index[f].append(rel_path)
    return index

def generate_diff(file1, file2, use_state_machine):
    lines1, orig1 = parse_file(file1, use_state_machine)
    lines2, orig2 = parse_file(file2, use_state_machine)
    differ = difflib.SequenceMatcher(None, lines1, lines2)
    diff = []
    def calculate_original_line_map(lines, orig_count):
        line_map = []
        current_line = 1
        for line in lines:
            while current_line <= orig_count and not line.strip():
                current_line += 1
            line_map.append(current_line)
            current_line += 1
        return line_map
    line_map1 = calculate_original_line_map(lines1, orig1)
    line_map2 = calculate_original_line_map(lines2, orig2)
    for tag, i1, i2, j1, j2 in differ.get_opcodes():
        if tag == "equal":
            continue
        if tag == "delete":
            for ln in range(i1, i2):
                diff.append(f"{Fore.RED}- [{line_map1[ln]:04}] {lines1[ln]}")
        elif tag == "insert":
            for ln in range(j1, j2):
                diff.append(f"{Fore.GREEN}+ [{line_map2[ln]:04}] {lines2[ln]}")
        elif tag == "replace":
            for ln in range(i1, i2):
                diff.append(f"{Fore.RED}- [{line_map1[ln]:04}] {lines1[ln]}")
            for ln in range(j1, j2):
                diff.append(f"{Fore.GREEN}+ [{line_map2[ln]:04}] {lines2[ln]}")
    return "\n".join(diff)

def calculate_similarity(lines1, lines2, orig1, orig2):
    set1 = set(lines1)
    set2 = set(lines2)
    common = len(set1 & set2)
    denominator = max(len(set1), len(set2)) or 1
    return {
        "ratio": common / denominator,
        "common": common,
        "orig1": orig1,
        "orig2": orig2,
        "valid1": len(set1),
        "valid2": len(set2),
    }

def file_compare(file1, file2, base_dir, show_diff, use_state_machine=False):
    lines1, orig1 = parse_file(file1, use_state_machine)
    lines2, orig2 = parse_file(file2, use_state_machine)
    result = calculate_similarity(lines1, lines2, orig1, orig2)
    result.update(
        {
            "diff": generate_diff(file1, file2, use_state_machine) if show_diff else "",
            "file_name": os.path.relpath(file1, base_dir),
            "path1": file1,
            "path2": file2,
        }
    )
    return result

def dir_compare(dir1, dir2, show_diff, ignore_dirs, ignore_files):
    index1 = build_index(dir1, ignore_dirs, ignore_files)
    index2 = build_index(dir2, ignore_dirs, ignore_files)
    total = defaultdict(int)
    details = []
    unmatched = {"source": [], "target": []}
    processed_pairs = set()
    all_files = set(index1.keys()).union(set(index2.keys()))
    for filename in all_files:
        paths1 = index1.get(filename, [])
        paths2 = index2.get(filename, [])
        if not paths2:
            unmatched["source"].extend([os.path.join(dir1, p) for p in paths1])
        if not paths1:
            unmatched["target"].extend([os.path.join(dir2, p) for p in paths2])
        for rel_path1 in paths1:
            full_path1 = os.path.join(dir1, rel_path1)
            for rel_path2 in paths2:
                full_path2 = os.path.join(dir2, rel_path2)
                sorted_pair = tuple(sorted([full_path1, full_path2]))
                if sorted_pair in processed_pairs:
                    continue
                processed_pairs.add(sorted_pair)
                res = file_compare(
                    full_path1, full_path2, os.path.dirname(full_path1), show_diff
                )
                res["file_name"] = f"{rel_path1} ↔ {rel_path2}"
                details.append(res)
                total["common"] += res["common"]
                total["valid1"] += res["valid1"]
                total["valid2"] += res["valid2"]
                total["orig1"] += res["orig1"]
                total["orig2"] += res["orig2"]
    unmatched["source"] = list(set(unmatched["source"]))
    unmatched["target"] = list(set(unmatched["target"]))
    denominator = max(total["valid1"], total["valid2"]) or 1
    total["ratio"] = total["common"] / denominator
    return total, details, unmatched

def print_summary(total, details, unmatched, color):
    """简化版结果显示"""
    print(f"\n{color('cyan')}=== 统计摘要 ===")
    print(f"{color('green')}▏ 匹配文件对: {len(details)} 对")
    print(f"{color('yellow')}▏ 源目录独有: {len(unmatched['source'])} 个")
    print(f"{color('yellow')}▏ 目标目录独有: {len(unmatched['target'])} 个")
    print(f"\n{color('cyan')}=== 综合分析 ===")
    print(f"{color('green')}▏ 重复总行数: {total['common']}")
    print(f"{color('magenta')}▏ 总原始行数: {total['orig1']} → 有效 {total['valid1']}")
    print(f"{color('magenta')}▏ 总目标行数: {total['orig2']} → 有效 {total['valid2']}")
    print(f"{color('yellow')}▏ 综合重复率: {total['ratio']:.2%} (基于有效行数)")

def print_detail(total, details, unmatched, show_detail, use_color=True):
    """增强版结果显示"""
    color = lambda c: getattr(Fore, c.upper()) if use_color else ""

    if not show_detail:
        print_summary(total, details, unmatched, color)
        return

    # 未匹配文件
    print(f"\n{color('cyan')}=== 未匹配文件 ===")

    if unmatched["source"]:
        print(f"\n{color('yellow')}▏ 源目录独有文件 ({len(unmatched['source'])} 个):")
        for path in unmatched["source"]:
            print(
                f"  {color('magenta')}• {os.path.relpath(path, os.path.dirname(path))}"
            )

    if unmatched["target"]:
        print(f"\n{color('yellow')}▏ 目标目录独有文件 ({len(unmatched['target'])} 个):")
        for path in unmatched["target"]:
            print(
                f"  {color('magenta')}• {os.path.relpath(path, os.path.dirname(path))}"
            )

    # 文件列表
    print(f"\n{color('cyan')}=== 匹配文件详情 ({len(details)} 个) ===")
    for info in sorted(details, key=lambda x: x["ratio"], reverse=True):
        ratio = info["ratio"]
        color_code = "GREEN" if ratio > 0.7 else "YELLOW" if ratio > 0.3 else "RED"

        line = (
            f"{color(color_code)}▏ {info['file_name']:55} "
            f"相似度: {ratio:.1%} "
            f"(原始: {info['orig1']:4}→{info['valid1']:4} | "
            f"目标: {info['orig2']:4}→{info['valid2']:4})"
        )
        print(line)

    print_summary(total, details, unmatched, color)

def parse_arguments():
    """解析命令行参数"""
    parser = argparse.ArgumentParser(description="高级代码相似性分析工具")
    parser.add_argument("source", help="源文件或目录路径")
    parser.add_argument("target", help="目标文件或目录路径")
    parser.add_argument("--diff", action="store_true", help="显示代码差异详情")
    parser.add_argument("--detail", action="store_true", help="显示文件差异详情")
    parser.add_argument(
        "--ignore-dirs",
        type=lambda x: [d.strip() for d in x.split(",") if d.strip()],
        default=[],
        help="要忽略的目录列表（多个目录用逗号分隔），如：--ignore-dirs venv,modules",
    )
    parser.add_argument(
        "--ignore-files",
        type=lambda x: [f.strip() for f in x.split(",") if f.strip()],
        default=[],
        help="要忽略的文件模式列表（多个模式用逗号分隔），如：--ignore-files *.tmp,*.bak",
    )
    parser.add_argument("--no-color", action="store_true", help="禁用颜色输出")
    parser.add_argument("--json", action="store_true", help="按json格式打印")

    args = parser.parse_args()

    # 确保忽略目录和文件模式列表中没有空值
    args.ignore_dirs = [d.strip() for d in args.ignore_dirs if d.strip()]
    args.ignore_files = [f.strip() for f in args.ignore_files if f.strip()]

    return args

def _on_json_output_exit():
    _orig_file_compare = globals().get('file_compare')
    _orig_dir_compare = globals().get('dir_compare')
    _orig_print = builtins.print
    _json_store = {'type': None, 'data': None}

    def _wrap_file_compare(file1, file2, base_dir, show_diff, use_state_machine=False):
        res = _orig_file_compare(file1, file2, base_dir, show_diff, use_state_machine)
        _json_store['type'] = 'file'
        _json_store['data'] = res
        return res

    def _wrap_dir_compare(dir1, dir2, show_diff, ignore_dirs, ignore_files):
        res = _orig_dir_compare(dir1, dir2, show_diff, ignore_dirs, ignore_files)
        _json_store['type'] = 'dir'
        total, details, unmatched = res
        _json_store['data'] = {'total': total, 'details': details, 'unmatched': unmatched}
        return res

    globals()['file_compare'] = _wrap_file_compare
    globals()['dir_compare'] = _wrap_dir_compare

    def _noop_print(*a, **kw): pass
    builtins.print = _noop_print

    def _print_json_at_exit():
        builtins.print = _orig_print
        data = _json_store['data'] if _json_store['type'] else {}
        _orig_print(json.dumps(data, ensure_ascii=False, indent=2))

    atexit.register(_print_json_at_exit)

def main():
    args = parse_arguments()

    try:
        use_color = not args.no_color
        if args.no_color:
            global Fore
            global Style
            Fore = EmptyStyle()
            Style = EmptyStyle()
        # 统一颜色处理函数
        color = lambda c: getattr(Fore, c.upper()) if use_color else ""

        if args.json:
            _on_json_output_exit()

        # 文件对比模式
        if os.path.isfile(args.source) and os.path.isfile(args.target):
            result = file_compare(
                args.source, args.target, os.path.dirname(args.source), args.diff
            )

            if args.diff and result["diff"]:
                print(f"\n{color('cyan')}=== 差异对比 ===")
                print(result["diff"])

            print(f"\n{color('cyan')}=== 文件分析 ===")
            print(f"{color('yellow')}▏ 重复率: {result['ratio']:.2%}")
            print(
                f"{color('magenta')}▏ 重复行数: {result['dup']} "
                f"(源: {result['total1']} | 目标: {result['total2']})"
            )

        # 目录对比模式
        elif os.path.isdir(args.source) and os.path.isdir(args.target):
            total, details, unmatched = dir_compare(  # 接收返回值
                args.source,
                args.target,
                args.diff,
                args.ignore_dirs,
                args.ignore_files,
            )
            if args.diff:
                print(f"\n{color('cyan')}=== 差异详情 ===")
                for file_info in details:
                    if file_info["diff"]:
                        print(f"\n{color('cyan')}--- {file_info['file_name']} ---")
                        print(file_info["diff"])

            # 传递unmatched参数
            print_detail(total, details, unmatched, args.detail, use_color)

        else:
            print(f"{color('red')}错误：需要两个文件或两个目录")

    except KeyboardInterrupt:
        print(f"\n{color('red')}操作已中止")
    except Exception as e:
        print(f"{color('red')}错误: {str(e)}")


if __name__ == "__main__":
    main()
