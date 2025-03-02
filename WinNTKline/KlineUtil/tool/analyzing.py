import os
import difflib
import argparse
from colorama import Fore, Style, init
from collections import defaultdict
import re
import fnmatch

# 初始化颜色支持
init(autoreset=True)

# 注释处理配置（支持多种编程语言）
COMMENT_PATTERNS = {
    ".py": [
        (r"#.*$", re.MULTILINE),  # 行注释
        (r"(\'\'\'(.*?)\'\'\'|\"\"\"(.*?)\"\"\")", re.DOTALL),  # 块注释
    ],
    ".java": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".js": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".cpp": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".c": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".rs": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".swift": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".kt": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".ts": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".css": [
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".scss": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".less": [
        (r"//.*$", re.MULTILINE),
        (r"/\*.*?\*/", re.DOTALL),
    ],
    ".html": [
        (r"<!--.*?-->", re.DOTALL),
    ],
    ".xml": [
        (r"<!--.*?-->", re.DOTALL),
    ],
}


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


def strip_comments(file_path, code):
    """高级注释去除方法，支持跨行处理"""
    file_ext = os.path.splitext(file_path)[1].lower()
    patterns = COMMENT_PATTERNS.get(file_ext, [])

    for pattern, flags in patterns:
        code = re.sub(pattern, "", code, flags=flags)

    # 处理通用情况
    code = re.sub(r"//.*$", "", code, flags=re.MULTILINE)
    code = re.sub(r"/\*.*?\*/", "", code, flags=re.DOTALL)
    code = re.sub(r"<!--.*?-->", "", code, flags=re.DOTALL)

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
    index = {}
    ignore_dirs = set(ignore_dirs)
    ignore_files = set(ignore_files)

    for root, dirs, files in os.walk(directory, topdown=True):
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
            index[rel_path] = path

    return index


def generate_diff(file1, file2):
    """生成带行号的彩色差异对比"""
    lines1, _ = read_file(file1)
    lines2, _ = read_file(file2)
    differ = difflib.SequenceMatcher(None, lines1, lines2)
    diff = []

    for tag, i1, i2, j1, j2 in differ.get_opcodes():
        if tag == "equal":
            continue

        if tag == "delete":
            for ln in range(i1, i2):
                diff.append(f"{Fore.RED}- [{ln+1:04}] {lines1[ln]}")
        elif tag == "insert":
            for ln in range(j1, j2):
                diff.append(f"{Fore.GREEN}+ [{ln+1:04}] {lines2[ln]}")
        elif tag == "replace":
            for ln in range(i1, i2):
                diff.append(f"{Fore.RED}- [{ln+1:04}] {lines1[ln]}")
            for ln in range(j1, j2):
                diff.append(f"{Fore.GREEN}+ [{ln+1:04}] {lines2[ln]}")

    return "\n".join(diff)


def calculate_similarity(lines1, lines2, orig1, orig2):
    """改进的相似度计算，分母使用原始行数"""
    counter = defaultdict(int)
    for line in lines1:
        counter[line] += 1

    common = 0
    for line in lines2:
        if counter.get(line, 0) > 0:
            common += 1
            counter[line] -= 1

    denominator = max(orig1, orig2) or 1
    return common / denominator, common, orig1, orig2


def file_compare(file1, file2, base_dir, show_diff):
    """执行文件比较"""
    lines1, orig1 = read_file(file1)
    lines2, orig2 = read_file(file2)

    ratio, dup, _, _ = calculate_similarity(lines1, lines2, orig1, orig2)
    diff_output = generate_diff(file1, file2) if show_diff else ""

    return {
        "ratio": ratio,
        "dup": dup,
        "total1": orig1,
        "total2": orig2,
        "diff": diff_output,
        "file_name": os.path.relpath(file1, base_dir),
        "path1": file1,
        "path2": file2,
    }


def dir_compare(dir1, dir2, show_diff, ignore_dirs, ignore_files):
    """带忽略规则的目录对比"""
    index1 = build_index(dir1, ignore_dirs, ignore_files)
    index2 = build_index(dir2, ignore_dirs, ignore_files)
    common_files = set(index1.keys()) & set(index2.keys())

    total = defaultdict(int)
    details = []

    for rel_path in common_files:
        res = file_compare(index1[rel_path], index2[rel_path], dir1, show_diff)
        total["dup"] += res["dup"]
        total["total1"] += res["total1"]
        total["total2"] += res["total2"]
        details.append(res)

    max_total = max(total["total1"], total["total2"]) or 1
    total["ratio"] = total["dup"] / max_total
    return total, details


def print_results(total, details, use_color=True):
    """打印带颜色的分析结果"""
    color = lambda c: getattr(Fore, c.upper()) if use_color else ""

    # 文件列表
    print(f"\n{color('cyan')}=== 文件详情 ({len(details)} 个) ===")
    for info in sorted(details, key=lambda x: x["ratio"], reverse=True):
        ratio = info["ratio"]
        if ratio > 0.7:
            c = "GREEN"
        elif ratio > 0.3:
            c = "YELLOW"
        else:
            c = "RED"
        print(f"{color(c)}▏ {info['file_name']:60} {ratio:.1%}")

    # 综合分析
    print(f"\n{color('cyan')}=== 综合分析 ===")
    print(f"{color('yellow')}▏ 匹配文件数: {len(details)}")
    print(f"{color('yellow')}▏ 综合重复率: {total['ratio']:.2%}")
    print(
        f"{color('magenta')}▏ 总重复行数: {total['dup']} "
        f"(源: {total['total1']} | 目标: {total['total2']})"
    )


def main():
    parser = argparse.ArgumentParser(description="高级代码相似性分析工具")
    parser.add_argument("source", help="源文件或目录路径")
    parser.add_argument("target", help="目标文件或目录路径")
    parser.add_argument("--diff", action="store_true", help="显示差异详情")
    parser.add_argument(
        "--ignore-dirs",
        nargs="+",
        default=[],
        help="要忽略的目录列表，如：venv node_modules",
    )
    parser.add_argument(
        "--ignore-files",
        nargs="+",
        default=[],
        help="要忽略的文件模式，如：*.tmp *.bak",
    )
    parser.add_argument("--no-color", action="store_true", help="禁用颜色输出")
    args = parser.parse_args()

    try:
        use_color = not args.no_color
        if args.no_color:
            # 修正类型错误：使用空类替代颜色属性
            global Fore, Style
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
            total, details = dir_compare(
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

            print_results(total, details, use_color)

        else:
            print(f"{color('red')}错误：需要两个文件或两个目录")

    except KeyboardInterrupt:
        print(f"\n{color('red')}操作已中止")
    except Exception as e:
        print(f"{color('red')}错误: {str(e)}")


if __name__ == "__main__":
    main()
