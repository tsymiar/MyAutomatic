import re
import argparse
from colorama import Fore, Style, init
from bs4 import BeautifulSoup

class TerminalRenderer:
    def __init__(self):
        init(autoreset=True)
        self.supported_formats = {"bold", "italic", "code"}
        self.element_handlers = {
            "heading": self.render_heading,
            "list": self.render_list,
            "blockquote": self.render_blockquote,
            "table": self.render_table,
            "md_table": self.render_md_table,  # markdown 表格转换支持
            "link": self.render_link,
            "image": self.render_image,
            "code_block": self.render_code_block,
            "html_comment": self.render_html_comment,
            "paragraph": self.render_paragraph,
        }

    def format_markdown(self, text):
        # 删除线~~text~~（仅显示内容，DIM效果）
        text = re.sub(r"~~(.+?)~~", lambda m: f"{Style.DIM}{m.group(1)}{Style.RESET_ALL}", text)
        # 加粗：去除 ** 符号
        text = re.sub(r"\*\*(.+?)\*\*", lambda m: f"{Style.BRIGHT}{m.group(1)}{Style.RESET_ALL}", text)
        # 斜体：去除 * 符号
        text = re.sub(r"\*(.+?)\*", lambda m: f"{Style.DIM}{m.group(1)}{Style.RESET_ALL}", text)
        # 斜体：支持 _text_ 格式
        text = re.sub(r"_(.+?)_", lambda m: f"{Style.DIM}{m.group(1)}{Style.RESET_ALL}", text)
        # 内联代码：去除反引号，仅显示内容（绿色显示）
        text = re.sub(r"`(.+?)`", lambda m: f"{Fore.GREEN}{m.group(1)}{Style.RESET_ALL}", text)
        # [![]()]()转换：显示图片 alt，链接 URL 蓝色显示
        text = re.sub(r"\[!\[(.*?)\]\((.*?)\)\]\((.*?)\)",
                      lambda m: f"{Style.BRIGHT}{m.group(1)}{Style.RESET_ALL} ({Fore.BLUE}{m.group(3)}{Style.RESET_ALL})",
                      text)
        return text

    def render_heading(self, element):
        level = element["level"]
        color_map = {1: Fore.RED, 2: Fore.CYAN, 3: Fore.MAGENTA,
                     4: Fore.BLUE, 5: Fore.GREEN, 6: Fore.YELLOW}
        color = color_map.get(level, Fore.WHITE)
        text = self.format_markdown(element["text"])
        return f"{color}{text}{Style.RESET_ALL}"

    def render_list(self, element):
        indent = "  " * element["depth"]
        bullet = f"{element.get('number', 1)}." if element["list_type"] == "ordered" else "•"
        text = self.format_markdown(element["text"])
        return f"{indent}{bullet} {text}"

    def render_blockquote(self, element):
        text = self.format_markdown(element["text"])
        return f"{Fore.YELLOW}> {text}{Style.RESET_ALL}"

    def render_table(self, element):
        headers = element.get("headers", [])
        rows = element.get("rows", [])
        # 如果行数中出现比 headers 更多的列，按最大列数处理
        max_columns = max(len(headers), max((len(row) for row in rows), default=0))
        # 补齐 headers 如不足 max_columns
        if len(headers) < max_columns:
            headers.extend([""] * (max_columns - len(headers)))
        # 动态确定每列宽度
        col_widths = [len(str(headers[i])) for i in range(max_columns)]
        for row in rows:
            for i in range(max_columns):
                cell = row[i] if i < len(row) else ""
                col_widths[i] = max(col_widths[i], len(str(cell)))
        horizontal_border = "+" + "+".join(["-" * (w + 2) for w in col_widths]) + "+"
        table_lines = [horizontal_border]
        header_cells = [f" {str(headers[i]).center(col_widths[i])} " for i in range(max_columns)]
        table_lines.append(f"|{'|'.join(header_cells)}|")
        table_lines.append(horizontal_border)
        for row in rows:
            cells = [f" {str(row[i]).ljust(col_widths[i])} " if i < len(row) else " " * (col_widths[i] + 2)
                     for i in range(max_columns)]
            table_lines.append(f"|{'|'.join(cells)}|")
        table_lines.append(horizontal_border)
        return "\n".join(table_lines)

    def render_md_table(self, element):
        # 直接复用 render_table 的格式化方案
        return self.render_table(element)

    def render_link(self, element):
        text = self.format_markdown(element["text"])
        return f"{Style.BRIGHT}{text}{Style.RESET_ALL} ({Fore.BLUE}{element['url']}{Style.RESET_ALL})"

    def render_image(self, element):
        return f"{Fore.MAGENTA}[🖼 {element['alt']}]{Style.RESET_ALL} ({Fore.BLUE}{element['src']}{Style.RESET_ALL})"

    def render_code_block(self, element):
        border = "┌" + "─" * 48 + "┐"
        code_lines = [f"│ {line}" for line in element["content"].split("\n")]
        return f"{border}\n" + "\n".join(code_lines) + f"\n{'└' + '─' * 48 + '┘'}"

    def render_html_comment(self, element):
        return f"{Fore.LIGHTBLACK_EX}※ {element['content']} ※{Style.RESET_ALL}"

    def render_paragraph(self, element):
        text = self.format_markdown(element["text"])
        return f"{text}"

class MarkdownHTMLParser:
    def transform_content(self, content):
        """
        Convert HTML to terminal formatted markdown text, 
        replacing HTML tags with styled output and handling code blocks and tables.
        """
        soup = BeautifulSoup(content, "html.parser")
        renderer = TerminalRenderer()

        # Replace HTML links
        for tag in soup.find_all("a"):
            tag.replace_with(renderer.render_link({
                "text": tag.get_text(strip=True),
                "url": tag.get("href", "")
            }))
        # Replace HTML images
        for tag in soup.find_all("img"):
            tag.replace_with(renderer.render_image({
                "alt": tag.get("alt", ""),
                "src": tag.get("src", "")
            }))
        # Replace HTML tables
        for tag in soup.find_all("table"):
            headers = [th.get_text(strip=True) for th in tag.find_all("th")]
            rows = []
            for tr in tag.find_all("tr"):
                cells = [td.get_text(strip=True) for td in tr.find_all("td")]
                if cells:
                    rows.append(cells)
            if headers or rows:
                tag.replace_with(renderer.render_table({"headers": headers, "rows": rows}))

        full_text = soup.get_text(separator="\n")
        lines = full_text.split("\n")
        result_lines = []
        i = 0

        while i < len(lines):
            line = lines[i]
            # Process code blocks
            if line.strip().startswith("```"):
                code_lines = []
                i += 1
                while i < len(lines) and not lines[i].strip().startswith("```"):
                    code_lines.append(lines[i])
                    i += 1
                result_lines.append(renderer.render_code_block({
                    "type": "code_block",
                    "content": "\n".join(code_lines)
                }))
                i += 1
            # Process markdown tables
            elif line.strip().startswith("|") and line.strip().endswith("|"):
                table_lines = []
                while i < len(lines) and lines[i].strip().startswith("|") and lines[i].strip().endswith("|"):
                    table_lines.append(lines[i].strip())
                    i += 1
                if len(table_lines) >= 2 and re.match(r"^\|\s*-+\s*(\|\s*-+\s*)+\|$", table_lines[1]):
                    headers = [cell.strip() for cell in table_lines[0].strip("|").split("|")]
                    data_rows = [[cell.strip() for cell in row.strip("|").split("|")] for row in table_lines[2:]]
                else:
                    headers = [cell.strip() for cell in table_lines[0].strip("|").split("|")]
                    data_rows = [[cell.strip() for cell in row.strip("|").split("|")] for row in table_lines[1:]]
                result_lines.append(renderer.render_md_table({
                    "type": "md_table",
                    "headers": headers,
                    "rows": data_rows
                }))
            # Process headings and paragraphs
            else:
                match = re.match(r"^(#{1,6})\s+(.+)$", line)
                if match:
                    result_lines.append(renderer.render_heading({
                        "type": "heading",
                        "level": len(match.group(1)),
                        "text": match.group(2).strip()
                    }))
                else:
                    result_lines.append(renderer.format_markdown(line))
                i += 1

        return "\n".join(result_lines)

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="终端文档格式化工具")
    arg_parser.add_argument("file", help="输入文件路径")
    args = arg_parser.parse_args()

    try:
        with open(args.file, "r", encoding="utf-8") as f:
            content = f.read()
        parser = MarkdownHTMLParser()
        output = parser.transform_content(content)
        print(output)
    except Exception as e:
        print(f"{Fore.RED}[错误: {str(e)}]{Style.RESET_ALL}")
