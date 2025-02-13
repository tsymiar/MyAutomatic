import os
import difflib
import argparse
from colorama import Fore, Style, init
from collections import defaultdict

# 初始化颜色支持
init(autoreset=True)


def is_text_file(file_path):
    """判断是否为文本文件"""
    text_exts = {
        ".py",
        ".txt",
        ".md",
        ".java",
        ".c",
        ".cpp",
        ".h",
        ".hpp",
        ".cs",
        ".go",
        ".rs",
        ".swift",
        ".kt",
        ".js",
        ".ts",
        ".html",
        ".css",
        ".scss",
        ".less",
        ".json",
        ".yml",
        ".yaml",
        ".xml",
    }
    return os.path.splitext(file_path)[1].lower() in text_exts


def read_file(file_path):
    """安全读取文件内容"""
    try:
        if not is_text_file(file_path):
            return []
        with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
            return [line.rstrip("\n") for line in f]
    except Exception as e:
        print(f"{Fore.RED}文件读取错误 {os.path.basename(file_path)}: {str(e)}")
        return []


def generate_diff(file1, file2):
    """生成带行号的彩色差异对比"""
    lines1 = read_file(file1)
    lines2 = read_file(file2)

    differ = difflib.SequenceMatcher(None, lines1, lines2)
    diff = []

    for tag, i1, i2, j1, j2 in differ.get_opcodes():
        if tag == "equal":
            continue

        # 处理不同操作类型
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


def calculate_similarity(lines1, lines2):
    """计算文件相似度"""
    valid1 = [ln.strip() for ln in lines1 if ln.strip()]
    valid2 = [ln.strip() for ln in lines2 if ln.strip()]

    counter1 = defaultdict(int)
    counter2 = defaultdict(int)
    for line in valid1:
        counter1[line] += 1
    for line in valid2:
        counter2[line] += 1

    common = sum(min(counter1[k], counter2[k]) for k in counter1 if k in counter2)
    max_lines = max(len(valid1), len(valid2)) or 1
    return common / max_lines, common, len(valid1), len(valid2)


def file_compare(file1, file2, base_dir, show_diff):
    """执行文件比较"""
    lines1 = read_file(file1)
    lines2 = read_file(file2)

    ratio, dup, total1, total2 = calculate_similarity(lines1, lines2)
    diff_output = generate_diff(file1, file2) if show_diff else ""

    return {
        "ratio": ratio,
        "dup": dup,
        "total1": total1,
        "total2": total2,
        "diff": diff_output,
        "file_name": os.path.relpath(file1, base_dir),
        "path1": file1,
        "path2": file2,
    }


def dir_compare(dir1, dir2, show_diff):
    """执行目录比较"""

    def build_index(directory):
        index = {}
        for root, _, files in os.walk(directory):
            for f in files:
                if not is_text_file(f):
                    continue
                rel_path = os.path.relpath(os.path.join(root, f), directory)
                index[rel_path] = os.path.join(root, f)
        return index

    index1 = build_index(dir1)
    index2 = build_index(dir2)
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
    color_map = {
        "cyan": Fore.CYAN if use_color else "",
        "yellow": Fore.YELLOW if use_color else "",
        "magenta": Fore.MAGENTA if use_color else "",
        "green": Fore.GREEN if use_color else "",
        "red": Fore.RED if use_color else "",
        "reset": Style.RESET_ALL if use_color else "",
    }

    # 打印综合分析
    print(f"\n{color_map['cyan']}=== 综合分析结果 ===")
    print(f"{color_map['yellow']}▏ 匹配文件数: {len(details)}")
    print(f"{color_map['yellow']}▏ 综合重复率: {total['ratio']:.2%}")
    print(
        f"{color_map['magenta']}▏ 总重复行数: {total['dup']} "
        f"(源: {total['total1']} | 目标: {total['total2']}){color_map['reset']}"
    )

    # 打印文件列表
    print(f"\n{color_map['cyan']}=== 文件详情 ({len(details)} 个) ===")
    for file_info in sorted(details, key=lambda x: x["ratio"], reverse=True):
        ratio = file_info["ratio"]
        if ratio > 0.7:
            color = color_map["green"]
        elif ratio > 0.3:
            color = color_map["yellow"]
        else:
            color = color_map["red"]

        print(f"{color}▏ {file_info['file_name']:60} {ratio:.1%}")


def main():
    parser = argparse.ArgumentParser(description="代码相似性分析工具")
    parser.add_argument("source", help="源文件或目录路径")
    parser.add_argument("target", help="目标文件或目录路径")
    parser.add_argument("--diff", action="store_true", help="显示差异详情")
    parser.add_argument("--no-color", action="store_true", help="禁用颜色输出")
    args = parser.parse_args()

    try:
        # 处理颜色选项
        use_color = not args.no_color
        if args.no_color:
            global Fore, Style
            Fore = type("", (), {k: "" for k in dir(Fore)})()
            Style = type("", (), {k: "" for k in dir(Style)})()

        # 执行比较
        if os.path.isfile(args.source) and os.path.isfile(args.target):
            result = file_compare(
                args.source, args.target, os.path.dirname(args.source), args.diff
            )

            if args.diff and result["diff"]:
                print(f"\n{Fore.CYAN if use_color else ''}=== 差异对比 ===")
                print(result["diff"])

            print(f"\n{Fore.CYAN if use_color else ''}=== 文件分析 ===")
            print(f"{Fore.YELLOW if use_color else ''}▏ 重复率: {result['ratio']:.2%}")
            print(
                f"{Fore.MAGENTA if use_color else ''}▏ 重复行数: {result['dup']} "
                f"(源: {result['total1']} | 目标: {result['total2']})"
            )

        elif os.path.isdir(args.source) and os.path.isdir(args.target):
            total, details = dir_compare(args.source, args.target, args.diff)

            if args.diff:
                print(f"\n{Fore.CYAN if use_color else ''}=== 差异详情 ===")
                for file_info in details:
                    if file_info["diff"]:
                        print(
                            f"\n{Fore.CYAN if use_color else ''}--- {file_info['file_name']} ---"
                        )
                        print(file_info["diff"])

            print_results(total, details, use_color)

        else:
            print(f"{Fore.RED if use_color else ''}错误：需要两个文件或两个目录")

    except KeyboardInterrupt:
        print(f"\n{Fore.RED if use_color else ''}操作已中止")
    except Exception as e:
        print(f"{Fore.RED if use_color else ''}错误: {str(e)}")


if __name__ == "__main__":
    main()
