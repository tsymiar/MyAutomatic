import re
import argparse
from colorama import Fore, Back, Style, init


class MarkdownHighlighter:
    def __init__(self):
        init(autoreset=True)
        self.states = {
            "code_block": False,
            "math_block": False,
            "html_block": False,
            "table": {
                "active": False,
                "type": None,  # 'markdown' 或 'html'
                "headers": [],
                "rows": [],
                "current_row": [],
            },
            "list_level": 0,
            "list_counters": [],
        }

        # 样式配置
        self.html_style = {
            "tag": Fore.BLUE + Style.BRIGHT,
            "attr": Fore.YELLOW,
            "value": Fore.GREEN,
            "comment": Fore.LIGHTBLACK_EX + Style.DIM,
            "doctype": Fore.MAGENTA,
            "entity": Fore.CYAN,
        }

    def highlight(self, md_text):
        lines = md_text.split("\n")
        for idx, line in enumerate(lines):
            processed = self._process_line(idx, line, lines)
            if processed is not None:
                print(processed)

    def _process_line(self, idx, line, all_lines):
        handlers = [
            self._handle_code_blocks,
            self._handle_math_blocks,
            self._handle_html_blocks,
            self._handle_html_tables,
            self._handle_markdown_tables,
            self._handle_headers,
            self._handle_lists,
            self._handle_quotes,
            self._handle_horizontal_rules,
            self._handle_tasklists,
            self._handle_links,
            self._handle_images,
            self._handle_autolinks,
            self._handle_inline_code,
            self._handle_formatting,
        ]

        for handler in handlers:
            result = handler(idx, line, all_lines)
            if result is not None:
                return result
        return line

    def _apply_style(self, text, styles):
        return "".join(styles) + text + Style.RESET_ALL

    def _handle_math_blocks(self, idx, line, all_lines):
        if line.strip() == "$$":
            self.states["math_block"] = not self.states["math_block"]
            style = Fore.MAGENTA + Style.BRIGHT
            symbol = "╭" if self.states["math_block"] else "╯"
            return self._apply_style(f"{symbol} Math Block " + "─" * 50, [style])
        if self.states["math_block"]:
            formatted = re.sub(
                r"(\\\w+|\^|_)",
                lambda m: self._apply_style(m.group(1), [Fore.CYAN]),
                line,
            )
            return self._apply_style("│ " + formatted, [Fore.MAGENTA])
        return None

    def _handle_headers(self, idx, line, all_lines):
        if match := re.match(r"^(#{1,6})\s+(.*)", line):
            level = len(match.group(1))
            title = match.group(2)
            colors = [
                Fore.RED,
                Fore.YELLOW,
                Fore.GREEN,
                Fore.CYAN,
                Fore.BLUE,
                Fore.MAGENTA,
            ]
            style = colors[level - 1] + Style.BRIGHT
            return self._apply_style(f"{'▌'*level} {title} {'▌'*level}", [style])
        return None

    def _handle_lists(self, idx, line, all_lines):
        if match := re.match(r"^(\s*)([-*+]|\d+\.)\s+(.*)", line):
            indent = len(match.group(1))
            current_level = indent // 4  # 假设每级缩进4空格
            marker = match.group(2)
            content = match.group(3)

            while len(self.states["list_counters"]) <= current_level:
                self.states["list_counters"].append(1)

            if current_level < self.states["list_level"]:
                for level in range(
                    current_level + 1, len(self.states["list_counters"])
                ):
                    self.states["list_counters"][level] = 1

            if marker in ("-", "*", "+"):
                bullet = "•"
            else:
                bullet_number = self.states["list_counters"][current_level]
                bullet = f"{bullet_number}."
                self.states["list_counters"][current_level] += 1

            self.states["list_level"] = current_level
            indentation = " " * (current_level * 4)
            return indentation + self._apply_style(f"{bullet} {content}", [Fore.CYAN])
        return None

    def _handle_quotes(self, idx, line, all_lines):
        if match := re.match(r"^>\s*(.*)", line):
            quote = match.group(1)
            next_line = all_lines[idx + 1] if idx + 1 < len(all_lines) else ""
            prefix = "│" if re.match(r"^>", next_line) else "└"
            return self._apply_style(f"{Fore.GREEN}{prefix} {quote}", [])
        return None

    def _handle_horizontal_rules(self, idx, line, all_lines):
        if re.match(r"^[-*_]{3,}$", line):
            return self._apply_style("─" * 50, [Fore.WHITE, Style.DIM])
        return None

    def _handle_tasklists(self, idx, line, all_lines):
        if match := re.match(r"^(\s*[-*+])\s+\[(.)\]\s+(.*)", line):
            state = match.group(2).lower()
            icon = "✓" if state == "x" else "✗"
            color = Fore.GREEN if state == "x" else Fore.RED
            return " " * len(match.group(1)) + self._apply_style(
                f"{icon} {match.group(3)}", [color]
            )
        return None

    def _handle_links(self, idx, line, all_lines):
        return re.sub(
            r"\[([^]]+)\]\(([^)]+)\)",
            lambda m: self._apply_style(f"🔗 {m.group(1)}", [Fore.BLUE])
            + self._apply_style(f"({m.group(2)})", [Fore.CYAN]),
            line,
        )

    def _handle_images(self, idx, line, all_lines):
        return re.sub(
            r"!\[([^]]+)\]\(([^)]+)\)",
            lambda m: self._apply_style(f"🖼 {m.group(1)}", [Fore.MAGENTA]),
            line,
        )

    def _handle_autolinks(self, idx, line, all_lines):
        return re.sub(
            r"<(https?://[^>]+)>",
            lambda m: self._apply_style(m.group(1), [Fore.CYAN]),
            line,
        )

    def _handle_inline_code(self, idx, line, all_lines):
        return re.sub(
            r"`([^`]+)`",
            lambda m: self._apply_style(m.group(1), [Back.BLACK, Fore.WHITE]),
            line,
        )

    def _handle_formatting(self, idx, line, all_lines):
        formats = [
            (r"\*\*\*(.*?)\*\*\*", [Style.BRIGHT + Style.ITALIC, Fore.YELLOW]),
            (r"\*\*(.*?)\*\*", [Style.BRIGHT, Fore.YELLOW]),
            (r"\*(.*?)\*", [Style.ITALIC, Fore.YELLOW]),
            (r"~~(.*?)~~", [Style.DIM, Fore.RED]),
            (r"__(.*?)__", [Style.NORMAL, Fore.WHITE]),
        ]
        for pattern, style in formats:
            line = re.sub(pattern, lambda m: self._apply_style(m.group(1), style), line)
        return line

    def _handle_code_blocks(self, idx, line, all_lines):
        if line.startswith("```"):
            self.states["code_block"] = not self.states["code_block"]
            lang = line[3:].strip() or "text"
            border = "╭" if self.states["code_block"] else "╰"
            return self._apply_style(
                f"{border}─ Code ({lang}) ─{'─'*50}",
                [Back.BLACK, Fore.WHITE, Style.BRIGHT],
            )
        if self.states["code_block"]:
            return self._apply_style(f"│ {line}", [Back.BLACK, Fore.WHITE])
        return None

    # 新增HTML处理部分
    def _handle_html_blocks(self, idx, line, all_lines):
        """处理HTML块级元素并隐藏原始标签"""
        # 处理注释
        if comment_match := re.search(r"<!--(.*?)-->", line):
            return self._apply_style(
                f"📝 {comment_match.group(1)}", [Fore.LIGHTBLACK_EX]
            )

        # 处理文档声明
        if doctype_match := re.match(r"<!DOCTYPE\s+(.*?)>", line, re.I):
            return self._apply_style(f"📜 {doctype_match.group(1)}", [Fore.MAGENTA])

        # 处理超链接
        if link_match := re.search(r'<a\s+href=(["\'])(.*?)\1>(.*?)</a>', line, re.I):
            return self._apply_style(
                f"🔗 {link_match.group(3)}", [Fore.BLUE]
            ) + self._apply_style(f"({link_match.group(2)})", [Fore.CYAN])

        # 处理图片
        if img_match := re.search(r'<img\s+src=(["\'])(.*?)\1', line, re.I):
            return self._apply_style(f"🖼 {img_match.group(2)}", [Fore.MAGENTA])

        # 处理换行符
        line = re.sub(r"<br\s*/?>", "↵\n", line)
        return line if line else None

    def _handle_html_tables(self, idx, line, all_lines):
        """增强版HTML表格解析"""
        # 表格开始检测（支持带属性的table标签）
        if not self.states["table"]["active"]:
            if re.search(r"<\s*table\b", line, re.I):
                self.states["table"] = {
                    "active": True,
                    "type": "html",
                    "headers": [],
                    "rows": [],
                    "current_row": [],
                    "colspans": {},  # 新增跨列处理
                }
                return ""  # 隐藏起始标签

        if self.states["table"]["active"]:
            # 处理表头（支持带属性的th标签）
            if th_match := re.search(r"<\s*th\b[^>]*>(.*?)<\s*/\s*th\s*>", line, re.I):
                cell_content = self._clean_html_tags(th_match.group(1))
                self.states["table"]["headers"].append(cell_content.strip())

            # 处理表格数据（支持带属性的td标签）
            if td_match := re.search(r"<\s*td\b[^>]*>(.*?)<\s*/\s*td\s*>", line, re.I):
                # 提取跨列属性
                colspan = 1
                if colspan_match := re.search(
                    r"colspan\s*=\s*['\"]?(\d+)", td_match.group(0), re.I
                ):
                    colspan = int(colspan_match.group(1))

                cell_content = self._clean_html_tags(td_match.group(1))
                # 处理跨列情况
                if colspan > 1:
                    self.states["table"]["colspans"][
                        len(self.states["table"]["current_row"])
                    ] = colspan
                self.states["table"]["current_row"].append(cell_content.strip())

            # 新行开始（支持带属性的tr标签）
            if re.search(r"<\s*tr\b", line, re.I):
                self.states["table"]["current_row"] = []
                self.states["table"]["colspans"] = {}

            # 行结束
            if re.search(r"<\s*/\s*tr\s*>", line, re.I):
                # 处理跨列单元格
                processed_row = []
                for i, cell in enumerate(self.states["table"]["current_row"]):
                    if i in self.states["table"]["colspans"]:
                        processed_row.append(
                            cell + f" (colspan:{self.states['table']['colspans'][i]})"
                        )
                    else:
                        processed_row.append(cell)
                self.states["table"]["rows"].append(processed_row)
                self.states["table"]["current_row"] = []

            # 表格结束
            if re.search(r"<\s*/\s*table\s*>", line, re.I):
                table_output = self._render_html_table()
                self.states["table"] = {"active": False, "type": None}
                return table_output

            return ""  # 隐藏所有表格相关标签
        return None

    def _clean_html_tags(self, text):
        """清除HTML标签保留内容"""
        return re.sub(r"<[^>]+>", "", text)

    def _render_html_table(self):
        """增强版表格渲染"""
        headers = self.states["table"]["headers"]
        rows = self.states["table"]["rows"]

        # 自动检测列宽（考虑跨列情况）
        col_widths = []
        for i in range(
            len(headers) if headers else range(max(len(row) for row in rows))
        ):
            max_width = 0
            if headers and i < len(headers):
                max_width = len(headers[i])
            for row in rows:
                if i < len(row):
                    cell = re.sub(r"\s*\(colspan:\d+\)", "", row[i])  # 忽略跨列标记
                    max_width = max(max_width, len(cell))
            col_widths.append(max_width + 2)  # 增加边距

        # 构建表格框架
        top = "┌" + "┬".join("─" * w for w in col_widths) + "┐"
        sep = "├" + "┼".join("─" * w for w in col_widths) + "┤"
        bottom = "└" + "┴".join("─" * w for w in col_widths) + "┘"

        # 构建表头
        table_lines = [Fore.GREEN + top]
        if headers:
            header_cells = []
            for i, header in enumerate(headers):
                header_cells.append(f" {header.center(col_widths[i]-2)} ")
            table_lines.append(Fore.YELLOW + "│" + "│".join(header_cells) + "│")
            table_lines.append(Fore.CYAN + sep)

        # 构建数据行
        for row in rows:
            cells = []
            for i in range(len(col_widths)):
                cell = ""
                if i < len(row):
                    # 移除跨列标记
                    clean_cell = re.sub(r"\s*\(colspan:\d+\)", "", row[i])
                    cell = f" {clean_cell.ljust(col_widths[i]-2)} "
                else:
                    cell = " " * col_widths[i]
                cells.append(cell)
            table_lines.append("│" + "│".join(cells) + "│")

        table_lines.append(Fore.GREEN + bottom)
        return "\n".join(table_lines)

    def _handle_markdown_tables(self, idx, line, all_lines):
        """处理Markdown原生表格"""
        if re.match(r"^\|.+\|$", line):
            if not self.states["table"]["active"]:
                self.states["table"] = {
                    "active": True,
                    "type": "markdown",
                    "headers": [],
                    "rows": [],
                    "current_row": [],
                }
            self.states["table"]["rows"].append(line)
            return None
        elif (
            self.states["table"]["active"]
            and self.states["table"]["type"] == "markdown"
        ):
            return self._render_markdown_table()
        return None

    def _render_markdown_table(self):
        """渲染Markdown原生表格"""
        raw = self.states["table"]["rows"]
        headers = [c.strip() for c in raw[0].split("|")[1:-1]]
        rows = [[c.strip() for c in r.split("|")[1:-1]] for r in raw[2:]]

        # 计算列宽
        col_widths = [
            max(len(str(row[i])) for row in [headers] + rows)
            for i in range(len(headers))
        ]

        # 构建表格
        top = "┌" + "┬".join("─" * (w + 2) for w in col_widths) + "┐"
        sep = "├" + "┼".join("─" * (w + 2) for w in col_widths) + "┤"
        bottom = "└" + "┴".join("─" * (w + 2) for w in col_widths) + "┘"

        header_line = (
            "│"
            + "│".join(f" {h.center(w)} " for h, w in zip(headers, col_widths))
            + "│"
        )

        data_lines = [
            "│" + "│".join(f" {c.ljust(w)} " for c, w in zip(row, col_widths)) + "│"
            for row in rows
        ]

        table = (
            [Fore.GREEN + top, Fore.YELLOW + header_line, Fore.CYAN + sep]
            + data_lines
            + [Fore.GREEN + bottom]
        )
        self.states["table"] = {"active": False, "type": None}
        return "\n".join(table)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Markdown终端格式化工具")
    parser.add_argument("file", help="要处理的Markdown文件")
    args = parser.parse_args()

    try:
        with open(args.file, "r", encoding="utf-8") as f:
            content = f.read()

        highlighter = MarkdownHighlighter()
        highlighter.highlight(content)

    except Exception as e:
        print(f"{Fore.RED}错误: {str(e)}")
