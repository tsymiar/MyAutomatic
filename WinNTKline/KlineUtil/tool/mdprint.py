import re
import argparse
from colorama import Fore, Style, init
from bs4 import BeautifulSoup


class TerminalRenderer:
    def __init__(self):
        init(autoreset=True)
        self._init_regex_patterns()

    def _init_regex_patterns(self):
        """Initialize regular expressions for markdown formatting"""
        self.format_patterns = [
            (
                r"~~(.+?)~~",
                lambda m: f"{Style.DIM}{m.group(1)}{Style.RESET_ALL}",
            ),  # Strikethrough
            (
                r"\*\*(.+?)\*\*",
                lambda m: f"{Style.BRIGHT}{m.group(1)}{Style.RESET_ALL}",
            ),  # Bold
            (
                r"\*(.+?)\*",
                lambda m: f"{Style.DIM}{m.group(1)}{Style.RESET_ALL}",
            ),  # Italic *
            (
                r"_(.+?)_",
                lambda m: f"{Style.DIM}{m.group(1)}{Style.RESET_ALL}",
            ),  # Italic _
            (
                r"`(.+?)`",
                lambda m: f"{Fore.GREEN}{m.group(1)}{Style.RESET_ALL}",
            ),  # Inline code
            (
                r"\[!\[(.*?)\]\((.*?)\)\]\((.*?)\)",  # Image links
                lambda m: f"{Style.BRIGHT}{m.group(1)} ({Fore.BLUE}{m.group(3)}{Style.RESET_ALL})",
            ),
        ]

    def format_markdown(self, text):
        """Apply markdown formatting to text"""
        for pattern, replacement in self.format_patterns:
            text = re.sub(pattern, replacement, text)
        return text

    def render_heading(self, element):
        """Render markdown headings with level-based coloring"""
        level = element["level"]
        colors = [Fore.RED, Fore.CYAN, Fore.MAGENTA, Fore.BLUE, Fore.GREEN, Fore.YELLOW]
        return f"{colors[min(level-1, 5)]}{self.format_markdown(element['text'])}{Style.RESET_ALL}"


    def render_table(self, element):
        """Render tables with perfectly aligned columns"""
        headers = element.get("headers", [])
        rows = element.get("rows", [])

        # Convert all elements to strings and flatten data
        headers = [str(h) for h in headers]
        rows = [[str(cell) for cell in row] for row in rows]

        # Calculate max columns
        try:
            max_columns = max([len(headers)] + [len(row) for row in rows], default=0)
        except ValueError:
            max_columns = 0

        if max_columns == 0:
            return ""

        # Normalize data structure
        headers = headers + [""] * (max_columns - len(headers))
        rows = [row + [""] * (max_columns - len(row)) for row in rows]

        # Unified column width calculation
        col_widths = [0] * max_columns
        try:
            # Combine headers and rows for unified processing
            all_rows = [headers] + rows

            # Calculate max width for each column
            for i in range(max_columns):
                col_values = [row[i] for row in all_rows if i < len(row)]
                if col_values:
                    col_widths[i] = max(len(str(cell)) for cell in col_values)
                else:
                    col_widths[i] = 0

            # Ensure minimum width for empty columns
            col_widths = [max(w, 1) for w in col_widths]
        except Exception:
            col_widths = [15] * max_columns

        # Build table components
        try:
            # Generate horizontal border template
            horizontal = "+" + "+".join("-" * (w + 2) for w in col_widths) + "+"

            lines = [horizontal]

            # Process headers
            header_cells = [f" {h.center(col_widths[i])} " for i, h in enumerate(headers)]
            lines.append(f"|{'|'.join(header_cells)}|")
            lines.append(horizontal)

            # Process data rows
            for row in rows:
                cells = [f" {cell.ljust(col_widths[i])} " for i, cell in enumerate(row)]
                lines.append(f"|{'|'.join(cells)}|")
                lines.append(horizontal)  # Add border after each row

            return "\n".join(lines)
        except Exception as e:
            return f"{Fore.YELLOW}[Table Error: {e}]{Style.RESET_ALL}"

    def render_code_block(self, content):
        """Render code blocks with border formatting"""
        border = f"┌{'─'*48}┐"
        code_lines = [f"│ {line}" for line in content.split("\n")]
        return f"{border}\n" + "\n".join(code_lines) + f"\n└{'─'*48}┘"

    def render_link(self, element):
        """Format hyperlinks with underlined text and URL"""
        text = self.format_markdown(element["text"])
        return f"{Style.BRIGHT}{text}{Style.RESET_ALL} ({Fore.BLUE}{element['url']}{Style.RESET_ALL})"

    def render_image(self, element):
        """Format images with emoji and source URL"""
        return f"{Fore.MAGENTA}[🖼 {element['alt']}]{Style.RESET_ALL} ({Fore.BLUE}{element['src']}{Style.RESET_ALL})"


class MarkdownProcessor:
    def __init__(self):
        self.renderer = TerminalRenderer()

    def process(self, content):
        """Main processing pipeline"""
        soup = BeautifulSoup(content, "html.parser")
        self._replace_html_elements(soup)
        return self._parse_markdown(soup.get_text("\n"))

    def _replace_html_elements(self, soup):
        """Replace HTML tags with formatted text"""
        # Process links
        for tag in soup.find_all("a"):
            tag.replace_with(
                self.renderer.render_link(
                    {"text": tag.get_text(strip=True), "url": tag.get("href", "")}
                )
            )

        # Process images
        for tag in soup.find_all("img"):
            tag.replace_with(
                self.renderer.render_image(
                    {"alt": tag.get("alt", ""), "src": tag.get("src", "")}
                )
            )

        # Process tables
        for table in soup.find_all("table"):
            headers = [th.get_text(strip=True) for th in table.find_all("th")]
            rows = [
                [td.get_text(strip=True) for td in tr.find_all("td")]
                for tr in table.find_all("tr")
                if tr.find_all("td")
            ]
            table.replace_with(
                self.renderer.render_table({"headers": headers, "rows": rows})
            )

    def _parse_markdown(self, text):
        """Process markdown content"""
        lines, output, i = text.split("\n"), [], 0

        while i < len(lines):
            line = lines[i]
            if line.strip().startswith("```"):
                # Handle code blocks
                code_content = []
                i += 1  # Skip opening ```
                while i < len(lines) and not lines[i].strip().startswith("```"):
                    code_content.append(lines[i])
                    i += 1
                output.append(self.renderer.render_code_block("\n".join(code_content)))
                i += 1  # Skip closing ```
            elif line.strip().startswith("|"):
                # Collect raw table data
                table_raw = []
                while i < len(lines) and lines[i].strip().startswith("|"):
                    # Preserve original lines for subsequent processing
                    table_raw.append(lines[i].strip())
                    i += 1
                # Split cells and process
                table_data = [s.strip("|").split("|") for s in table_raw]
                output.append(self._process_table(table_data))
            elif line.startswith("#"):
                # Handle headings
                level = len(line) - len(line.lstrip("#"))
                output.append(
                    self.renderer.render_heading(
                        {"level": level, "text": line.lstrip("#").strip()}
                    )
                )
                i += 1
            else:
                # Handle regular text
                output.append(self.renderer.format_markdown(line))
                i += 1

        return "\n".join(output)

    def _process_table(self, rows):
        """Process markdown table data and skip alignment rows"""
        valid_rows = []

        # Filter alignment rows and empty lines
        for row in rows:
            if isinstance(row, list):
                line_str = "|".join(row)
            else:
                line_str = str(row)

            # Use regex to match Markdown table alignment rows
            if re.match(r"^[\s|:-]+$", line_str.strip()):
                continue
            if any(cell.strip() for cell in row):
                valid_rows.append(row)

        if not valid_rows:
            return ""

        # Extract headers
        headers = [cell.strip() for cell in valid_rows[0]]

        # Extract data rows
        body_rows = []
        for row in valid_rows[1:]:
            if len(row) == len(headers):
                body_rows.append([cell.strip() for cell in row])

        # Return rendered result when table is valid
        if len(headers) > 0 or len(body_rows) > 0:
            return self.renderer.render_table({"headers": headers, "rows": body_rows})
        return ""


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Terminal Markdown/HTML Renderer")
    parser.add_argument("file", help="Input file path")
    args = parser.parse_args()

    try:
        with open(args.file, "r", encoding="utf-8") as f:
            processed = MarkdownProcessor().process(f.read())
            print(processed)
    except FileNotFoundError:
        print(f"{Fore.RED}Error: File '{args.file}' not found{Style.RESET_ALL}")
    except Exception as e:
        print(f"{Fore.RED}Rendering Error: {str(e)}{Style.RESET_ALL}")
