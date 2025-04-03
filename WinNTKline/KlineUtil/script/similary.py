import os
import difflib
import argparse
import fnmatch
from colorama import Fore, init
from collections import defaultdict
import re

Style = None
# 初始化颜色支持
init(autoreset=True)

# 注释处理配置（支持多种编程语言）
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

# 按文件扩展名映射注释类型
COMMENT_PATTERNS = {
    ext: (COMMENT_TYPES["line"] + COMMENT_TYPES["block"])
    for ext in [
        ".py",
        ".java",
        ".js",
        ".cpp",
        ".c",
        ".rs",
        ".swift",
        ".kt",
        ".ts",
        ".css",
        ".scss",
        ".less",
        ".html",
        ".xml",
    ]
}
# 特殊处理纯文本类型
COMMENT_PATTERNS.update(
    {
        ".txt": [],
        ".md": [],
        ".json": [],
        ".yml": [],
        ".yaml": [],
    }
)


class EmptyStyle:
    """用于禁用颜色的空样式类"""

    def __getattr__(self, name):
        return ""


def is_text_file(file_path):
    """判断是否为支持的文本文件类型"""
    text_exts = set(COMMENT_PATTERNS.keys()).union(
        {".txt", ".md", ".json", ".yml", ".yaml"}
    )
    return os.path.splitext(file_path)[1].lower() in text_exts


# 定义类型别名
LineList = list[str]


def strip_comments(file_path: str, code: str) -> LineList:
    """
    Args:
        file_path: 文件路径，用于确定文件类型
        code: 源代码内容

    Returns:
        去除注释后的有效代码行列表
    """
    file_ext = os.path.splitext(file_path)[1].lower()
    patterns = COMMENT_PATTERNS.get(file_ext, [])

    # 应用所有匹配的注释模式
    for pattern, flags in patterns:
        code = re.sub(pattern, "", code, flags=flags)

    # 返回非空且去除右侧空格的代码行
    return [line.rstrip() for line in code.splitlines() if line.rstrip()]


def read_file(file_path):
    """读取文件并返回有效代码行和原始行数"""
    try:
        if not is_text_file(file_path):
            return [], 0
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read()
        original_lines = content.count("\n") + 1  # 准确统计原始行数
        return strip_comments(file_path, content), original_lines
    except Exception as e:
        print(f"{Fore.RED}文件读取错误 {os.path.basename(file_path)}: {str(e)}")
        return [], 0


def build_index(directory, ignore_dirs, ignore_files):
    """带目录和文件忽略功能的文件索引构建"""
    index = defaultdict(list)
    ignore_dirs = set(ignore_dirs)
    ignore_files = set(ignore_files)

    for root, dirs, files in os.walk(directory, True):
        # 过滤忽略目录
        dirs[:] = [d for d in dirs if d not in ignore_dirs]

        for f in files:
            # 跳过非文本文件
            if not is_text_file(f):
                continue
            # 检查文件是否匹配忽略模式
            if any(fnmatch.fnmatch(f, pattern) for pattern in ignore_files):
                continue
            path = os.path.join(root, f)
            rel_path = os.path.relpath(path, directory)
            index[f].append(rel_path)

    return index


def generate_diff(file1, file2):
    """生成带原始行号的彩色差异对比"""
    lines1, orig1 = read_file(file1)
    lines2, orig2 = read_file(file2)
    differ = difflib.SequenceMatcher(None, lines1, lines2)
    diff = []

    # 计算原始行号映射
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
    """使用有效行数计算相似度"""
    valid1 = len(lines1)
    valid2 = len(lines2)

    counter = defaultdict(int)
    for line in lines1:
        counter[line] += 1

    common = 0
    for line in lines2:
        if counter.get(line, 0) > 0:
            common += 1
            counter[line] -= 1

    denominator = max(valid1, valid2) or 1
    return {
        "ratio": common / denominator,
        "common": common,
        "orig1": orig1,
        "orig2": orig2,
        "valid1": valid1,
        "valid2": valid2,
    }


def file_compare(file1, file2, base_dir, show_diff):
    """更新文件比较逻辑"""
    lines1, orig1 = read_file(file1)
    lines2, orig2 = read_file(file2)

    result = calculate_similarity(lines1, lines2, orig1, orig2)
    result.update(
        {
            "diff": generate_diff(file1, file2) if show_diff else "",
            "file_name": os.path.relpath(file1, base_dir),
            "path1": file1,
            "path2": file2,
        }
    )
    return result


def dir_compare(dir1, dir2, show_diff, ignore_dirs, ignore_files):
    """更新目录对比统计"""
    index1 = build_index(dir1, ignore_dirs, ignore_files)
    index2 = build_index(dir2, ignore_dirs, ignore_files)

    total = defaultdict(int)
    details = []
    unmatched = {"source": [], "target": []}  # 未匹配文件存储
    processed_pairs = set()  # 文件对已处理

    # 获取所有文件名集合
    all_files = set(index1.keys()).union(set(index2.keys()))

    for filename in all_files:
        paths1 = index1.get(filename, [])
        paths2 = index2.get(filename, [])

        # 未匹配文件处理
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

    # 确保未匹配文件列表唯一
    unmatched["source"] = list(set(unmatched["source"]))
    unmatched["target"] = list(set(unmatched["target"]))

    denominator = max(total["valid1"], total["valid2"]) or 1
    total["ratio"] = total["common"] / denominator

    return total, details, unmatched  # 返回未匹配文件信息


def print_results(total, details, unmatched, use_color=True):
    """增强版结果显示"""
    color = lambda c: getattr(Fore, c.upper()) if use_color else ""

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

    # 统计摘要
    print(f"\n{color('cyan')}=== 统计摘要 ===")
    print(f"{color('green')}▏ 匹配文件对: {len(details)} 对")
    print(f"{color('yellow')}▏ 源目录独有: {len(unmatched['source'])} 个")
    print(f"{color('yellow')}▏ 目标目录独有: {len(unmatched['target'])} 个")
    # 综合分析
    print(f"\n{color('cyan')}=== 综合分析 ===")
    print(f"{color('green')}▏ 重复总行数: {total['common']}")
    print(f"{color('magenta')}▏ 总原始行数: {total['orig1']} → 有效 {total['valid1']}")
    print(f"{color('magenta')}▏ 总目标行数: {total['orig2']} → 有效 {total['valid2']}")
    print(f"{color('yellow')}▏ 综合重复率: {total['ratio']:.2%} (基于有效行数)")

def parse_arguments():
    """解析命令行参数"""
    parser = argparse.ArgumentParser(description="高级代码相似性分析工具")
    parser.add_argument("source", help="源文件或目录路径")
    parser.add_argument("target", help="目标文件或目录路径")
    parser.add_argument("--diff", action="store_true", help="显示差异详情")
    parser.add_argument(
        "--ignore-dirs",
        nargs="+",
        default=[],
        help="要忽略的目录列表（多个目录用空格分隔），如：--ignore-dirs venv modules",
    )
    parser.add_argument(
        "--ignore-files",
        nargs="+",
        default=[],
        help="要忽略的文件模式（多个模式用空格分隔），如：--ignore-files *.tmp *.bak",
    )
    parser.add_argument("--no-color", action="store_true", help="禁用颜色输出")

    args = parser.parse_args()

    # 确保忽略目录和文件模式列表中没有空值
    args.ignore_dirs = [d.strip() for d in args.ignore_dirs if d.strip()]
    args.ignore_files = [f.strip() for f in args.ignore_files if f.strip()]

    return args


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
            print_results(total, details, unmatched, use_color)

        else:
            print(f"{color('red')}错误：需要两个文件或两个目录")

    except KeyboardInterrupt:
        print(f"\n{color('red')}操作已中止")
    except Exception as e:
        print(f"{color('red')}错误: {str(e)}")


if __name__ == "__main__":
    main()
