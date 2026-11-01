// by: DeepSeek/GPT-4o
#pragma once
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <unistd.h>

constexpr const char* kAnsiReset = "\033[0m";

enum class BlockType {
    None,
    Heading,
    List,
    Paragraph,
    CodeBlock,
    Quote,
    Rule,
    Table
};

struct Block {
    BlockType type = BlockType::None;
    int heading_level = 0;
    int indent = 0;
    std::string marker{};
    std::string language{};
    std::vector<std::string> contents{};
};

// Unicode mathematical symbol mapping
const std::vector<std::pair<std::string, std::string>> SYMBOL_MAP = {
    {"\\int", "∫"}, {"\\infty", "∞"}, {"\\pi", "π"},
    {"\\sqrt", "√"}, {"\\sum", "Σ"},
    {"\\times", "×"}, {"\\cdot", "·"}, {"\\alpha", "α"},
    {"\\beta", "β"}, {"\\gamma", "γ"}, {"\\delta", "δ"},
    {"\\theta", "θ"}, {"\\lambda", "λ"}, {"\\mu", "μ"},
    {"\\sigma", "σ"}, {"\\omega", "ω"}, {"\\Delta", "Δ"},
    {"\\leq", "≤"}, {"\\geq", "≥"}, {"\\neq", "≠"},
    {"\\approx", "≈"}, {"\\pm", "±"}, {"\\rightarrow", "→"}
};

// Super/subscript glyphs (incl. Greek)
const std::vector<std::tuple<char, std::string, std::string>> SUPER_SUB_SCRIPT = {
    {'0', "⁰", "₀"}, {'1', "¹", "₁"}, {'2', "²", "₂"}, {'3', "³", "₃"},
    {'4', "⁴", "₄"}, {'5', "⁵", "₅"}, {'6', "⁶", "₆"}, {'7', "⁷", "₇"},
    {'8', "⁸", "₈"}, {'9', "⁹", "₉"}, {'-', "⁻", "₋"}, {'+', "⁺", "₊"},
    {'a', "ᵃ", "ₐ"}, {'b', "ᵇ", "♭"}, {'c', "ᶜ", "꜀"}, {'d', "ᵈ", "ꝱ"},
    {'e', "ᵉ", "ₑ"}, {'f', "ᶠ", "ꞙ"}, {'x', "ˣ", "ₓ"}, {'y', "ʸ", "ᵧ"},
    {'h', "ʰ", "ₕ"}, {'i', "ⁱ", "ᵢ"}, {'j', "ʲ", "ⱼ"}, {'k', "ᵏ", "ₖ"},
    {'l', "ˡ", "ₗ"}, {'m', "ᵐ", "ₘ"}, {'n', "ⁿ", "ₙ"}, {'o', "ᵒ", "ₒ"},
    {'p', "ᵖ", "ₚ"}, {'r', "ʳ", "ᵣ"}, {'s', "ˢ", "ₛ"}, {'t', "ᵗ", "ₜ"},
    {'u', "ᵘ", "ᵤ"}, {'v', "ᵛ", "ᵥ"}
};

inline void msWait(unsigned int tms)
{
    const int Thousand = 1000;
    struct timeval time;
    time.tv_sec = tms / Thousand;
    time.tv_usec = tms % Thousand * Thousand;
    select(0, NULL, NULL, NULL, &time);
}

inline std::string getFileContent(const std::string& filename)
{
    std::string content{};
    std::ifstream file(filename);
    if (file.is_open()) {
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    } else {
        return {};
    }
    file.close();
    if (content.empty()) {
        return {};
    }
    return content;
}

class Markdown {
public:
    Markdown() = default;

    void reset()
    {
        m_buffer.clear();
        m_table.clear();
        m_in_code = false;
    }

    // Render a line only once complete, so ** and ` survive chunk splits.
    std::string process_chunk(const std::string& chunk)
    {
        m_buffer += chunk;
        return drain(false);
    }

    std::string flush() { return drain(true); }

    std::vector<Block> parse_blocks(const std::string& content) const
    {
        std::vector<Block> blocks;
        Block cur{};
        bool in_code = false;

        for (const std::string& raw : list_lines(content)) {
            const std::string line = rtrim(raw);
            const std::string text = trim(line);

            if (is_code_fence(text)) {
                if (in_code) {
                    blocks.push_back(cur);
                    cur = Block{};
                } else {
                    push_block(cur, blocks);
                    cur.type = BlockType::CodeBlock;
                    cur.language = trim(text.substr(3));
                }
                in_code = !in_code;
                continue;
            }
            if (in_code) {
                cur.contents.push_back(line);
                continue;
            }
            if (text.empty()) {
                push_block(cur, blocks);
                continue;
            }
            if (is_rule(text)) {
                push_block(cur, blocks);
                Block rule{};
                rule.type = BlockType::Rule;
                blocks.push_back(rule);
                continue;
            }

            int level = heading_level(line);
            if (level > 0) {
                push_block(cur, blocks);
                Block head{};
                head.type = BlockType::Heading;
                head.heading_level = level;
                head.contents.push_back(trim(line.substr(line.find_first_not_of(" #"))));
                blocks.push_back(head);
                continue;
            }
            if (is_table_row(text)) {
                if (cur.type == BlockType::Table) {
                    cur.contents.push_back(text);
                } else {
                    push_block(cur, blocks);
                    cur.type = BlockType::Table;
                    cur.contents.push_back(text);
                }
                continue;
            }

            int indent = 0;
            std::string marker{};
            size_t body = 0;
            if (list_marker(line, indent, marker, body)) {
                push_block(cur, blocks);
                Block item{};
                item.type = BlockType::List;
                item.indent = indent;
                item.marker = marker;
                item.contents.push_back(trim(line.substr(body)));
                blocks.push_back(item);
                continue;
            }
            if (text[0] == '>') {
                if (cur.type == BlockType::Quote) {
                    cur.contents.push_back(trim(text.substr(1)));
                } else {
                    push_block(cur, blocks);
                    cur.type = BlockType::Quote;
                    cur.contents.push_back(trim(text.substr(1)));
                }
                continue;
            }
            if (cur.type == BlockType::Paragraph) {
                cur.contents.push_back(text);
            } else {
                push_block(cur, blocks);
                cur.type = BlockType::Paragraph;
                cur.contents.push_back(text);
            }
        }

        push_block(cur, blocks);
        return blocks;
    }

    static std::string Parse(const std::string& content, bool alpha = true)
    {
        Markdown md;
        std::string out;
        for (const Block& block : md.parse_blocks(content)) {
            switch (block.type) {
            case BlockType::Heading: {
                std::string style = heading_style(block.heading_level);
                if (block.heading_level <= 2) style += "\033[4m";
                out += style + md.render_inline(render_math(block.contents[0])) + kAnsiReset + "\n";
                if (alpha) out += "\n";
                break;
            }
            case BlockType::List:
                out += std::string(block.indent * 2, ' ') + "\033[32m" + block.marker + "\033[0m "
                    + md.render_inline(render_math(block.contents[0])) + "\n";
                break;
            case BlockType::Quote:
                for (const std::string& line : block.contents) {
                    out += "\033[36m┃ \033[0m" + md.render_inline(render_math(line)) + "\n";
                }
                if (alpha) out += "\n";
                break;
            case BlockType::Rule:
                out += "\033[90m" + repeat("─", terminal_width()) + "\033[0m\n";
                if (alpha) out += "\n";
                break;
            case BlockType::Table:
                out += md.render_table(block);
                if (alpha) out += "\n";
                break;
            case BlockType::CodeBlock:
                out += md.render_code(block);
                if (alpha) out += "\n";
                break;
            case BlockType::Paragraph:
                for (const std::string& line : block.contents) {
                    out += md.render_inline(render_math(line)) + "\n";
                }
                if (alpha) out += "\n";
                break;
            default:
                break;
            }
        }
        return out;
    }

private:
    static std::string repeat(const std::string& unit, size_t times)
    {
        std::string out;
        out.reserve(unit.size() * times);
        for (size_t i = 0; i < times; ++i) out += unit;
        return out;
    }

    struct InlineSpan {
        std::string open;
        std::string close;
        const char* style;
        bool verbatim;
        InlineSpan(std::string o, std::string c, const char* s, bool v)
            : open(std::move(o)), close(std::move(c)), style(s), verbatim(v) {}
    };

    std::string m_buffer{};
    std::vector<std::string> m_table{};
    bool m_in_code = false;

    static void push_block(Block& cur, std::vector<Block>& blocks)
    {
        if (cur.type != BlockType::None) {
            blocks.push_back(cur);
        }
        cur = Block{};
    }

    static const std::vector<InlineSpan>& inline_spans()
    {
        // Longer markers come first so ** wins over *.
        static const std::vector<InlineSpan> spans = {
            { "~~", "~~", "\033[9m", false },
            { "**", "**", "\033[1m", false },
            { "__", "__", "\033[1m", false },
            { "`", "`", "\033[36m", true },
            { "*", "*", "\033[3m", false },
            { "_", "_", "\033[3m", false }
        };
        return spans;
    }

    static const char* heading_style(int level)
    {
        static const char* styles[] = {
            "\033[1;35m", "\033[1;36m", "\033[1;34m", "\033[1;33m", "\033[1;32m", "\033[1;37m"
        };
        return (level >= 1 && level <= 6) ? styles[level - 1] : "\033[1m";
    }

    // Fill rules/borders to terminal width (fallback 80).
    static size_t terminal_width()
    {
        struct winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 20) {
            return ws.ws_col;
        }
        return 80;
    }

    static bool is_wide(unsigned int cp)
    {
        return (cp >= 0x1100 && cp <= 0x115F) ||
            (cp >= 0x2E80 && cp <= 0xA4CF) ||
            (cp >= 0xAC00 && cp <= 0xD7A3) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xFE30 && cp <= 0xFE6F) ||
            (cp >= 0xFF00 && cp <= 0xFF60) ||
            (cp >= 0xFFE0 && cp <= 0xFFE6) ||
            (cp >= 0x20000 && cp <= 0x3FFFD);
    }

    // CJK/wide glyphs count as 2 columns.
    static size_t display_width(const std::string& text)
    {
        size_t width = 0;
        for (size_t i = 0; i < text.size();) {
            unsigned char c = static_cast<unsigned char>(text[i]);
            if (c < 0x80) {
                ++width;
                ++i;
                continue;
            }
            size_t len = 0;
            unsigned int cp = 0;
            if ((c & 0xE0) == 0xC0) { len = 2; cp = c & 0x1Fu; }
            else if ((c & 0xF0) == 0xE0) { len = 3; cp = c & 0x0Fu; }
            else if ((c & 0xF8) == 0xF0) { len = 4; cp = c & 0x07u; }
            if (len == 0 || i + len > text.size()) {
                ++width;
                ++i;
                continue;
            }
            for (size_t k = 1; k < len; ++k) {
                cp = (cp << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3Fu);
            }
            i += len;
            width += is_wide(cp) ? 2 : 1;
        }
        return width;
    }

    static std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    static std::string rtrim(const std::string& s)
    {
        size_t end = s.find_last_not_of(" \t\r");
        return end == std::string::npos ? "" : s.substr(0, end + 1);
    }

    static bool is_code_fence(const std::string& line)
    {
        return line.compare(0, 3, "```") == 0;
    }

    static bool is_rule(const std::string& line)
    {
        if (line.size() < 3) return false;
        char c = line[0];
        if (c != '-' && c != '*' && c != '_') return false;
        size_t count = 0;
        for (char ch : line) {
            if (ch == c) ++count;
            else if (ch != ' ') return false;
        }
        return count >= 3;
    }

    static int heading_level(const std::string& line)
    {
        size_t pos = line.find_first_not_of(' ');
        if (pos == std::string::npos || line[pos] != '#') return 0;
        int level = 0;
        while (pos < line.size() && line[pos] == '#' && level < 6) {
            ++level;
            ++pos;
        }
        return (pos < line.size() && line[pos] == ' ') ? level : 0;
    }

    static bool list_marker(const std::string& line, int& indent, std::string& marker, size_t& body)
    {
        size_t pos = line.find_first_not_of(' ');
        if (pos == std::string::npos) return false;
        indent = static_cast<int>(pos / 2);
        char c = line[pos];
        if (c == '-' || c == '*' || c == '+') {
            if (pos + 1 >= line.size() || line[pos + 1] != ' ') return false;
            marker = "•";
            body = pos + 2;
            return true;
        }
        if (isdigit(static_cast<unsigned char>(c))) {
            size_t p = pos;
            while (p < line.size() && isdigit(static_cast<unsigned char>(line[p]))) ++p;
            if (p < line.size() && (line[p] == '.' || line[p] == ')') &&
                p + 1 < line.size() && line[p + 1] == ' ') {
                marker = line.substr(pos, p - pos + 1);
                body = p + 2;
                return true;
            }
        }
        return false;
    }

    static bool is_table_row(const std::string& line)
    {
        return line.size() >= 2 && line.front() == '|' && line.back() == '|';
    }

    static bool is_table_divider(const std::string& line)
    {
        bool has_dash = false;
        for (char c : line) {
            if (c == '-') has_dash = true;
            else if (c != '|' && c != ':' && c != ' ') return false;
        }
        return has_dash;
    }

    static std::vector<std::string> split_cells(const std::string& row)
    {
        std::vector<std::string> cells;
        std::string cell;
        const std::string body = row.substr(1, row.size() - 2);
        for (size_t i = 0; i < body.size(); ++i) {
            if (body[i] == '\\' && i + 1 < body.size() && body[i + 1] == '|') {
                cell += '|';
                ++i;
                continue;
            }
            if (body[i] == '|') {
                cells.push_back(trim(cell));
                cell.clear();
                continue;
            }
            cell += body[i];
        }
        cells.push_back(trim(cell));
        return cells;
    }

    // Backtick spans are emitted verbatim.
    std::string render_inline(const std::string& text) const
    {
        std::string out;
        size_t i = 0;
        while (i < text.size()) {
            if (text[i] == '\\' && i + 1 < text.size()) {
                out += text[i + 1];
                i += 2;
                continue;
            }
            if (text[i] == '[') {
                size_t label_end = text.find(']', i);
                if (label_end != std::string::npos && label_end + 1 < text.size() &&
                    text[label_end + 1] == '(') {
                    size_t url_end = text.find(')', label_end + 2);
                    if (url_end != std::string::npos) {
                        out += "\033[34m" + render_inline(text.substr(i + 1, label_end - i - 1)) + kAnsiReset
                            + " (\033[4;90m" + text.substr(label_end + 2, url_end - label_end - 2) + kAnsiReset + ")";
                        i = url_end + 1;
                        continue;
                    }
                }
            }
            bool matched = false;
            for (const InlineSpan& span : inline_spans()) {
                if (text.compare(i, span.open.size(), span.open) != 0) continue;
                size_t body_start = i + span.open.size();
                size_t end = text.find(span.close, body_start);
                if (end == std::string::npos || end == body_start) continue;
                std::string body = text.substr(body_start, end - body_start);
                if (body.empty() ||
                    isspace(static_cast<unsigned char>(body.front())) ||
                    isspace(static_cast<unsigned char>(body.back()))) {
                    continue; // not emphasis (e.g. "2 * 3 * 4")
                }
                out += span.style + (span.verbatim ? body : render_inline(body)) + kAnsiReset;
                i = end + span.close.size();
                matched = true;
                break;
            }
            if (!matched) {
                out += text[i];
                ++i;
            }
        }
        return out;
    }

    std::string render_table(const Block& block) const
    {
        std::vector<std::vector<std::string>> rows;
        for (const std::string& line : block.contents) {
            if (is_table_divider(line)) continue;
            rows.push_back(split_cells(line));
        }
        if (rows.empty()) return "";

        size_t cols = 0;
        for (const std::vector<std::string>& row : rows) {
            cols = std::max(cols, row.size());
        }
        std::vector<size_t> width(cols, 0);
        for (const std::vector<std::string>& row : rows) {
            for (size_t i = 0; i < cols; ++i) {
                size_t len = i < row.size() ? display_width(row[i]) : 0;
                width[i] = std::max(width[i], len);
            }
        }

        std::string out;
        for (size_t r = 0; r < rows.size(); ++r) {
            out += "\033[90m│\033[0m";
            for (size_t i = 0; i < cols; ++i) {
                std::string cell = i < rows[r].size() ? rows[r][i] : "";
                std::string shown = r == 0 ? ("\033[1m" + render_inline(cell) + kAnsiReset) : render_inline(cell);
                out += " " + shown + std::string(width[i] - display_width(cell), ' ') + " \033[90m│\033[0m";
            }
            out += "\n";
            if (r == 0) {
                out += "\033[90m├";
                for (size_t i = 0; i < cols; ++i) {
                    out += repeat("─", width[i] + 2);
                    if (i + 1 < cols) out += "┼";
                }
                out += "┤\033[0m\n";
            }
        }
        return out;
    }

    std::string render_code(const Block& block) const
    {
        const std::string lang = normalize_lang(block.language);
        const std::string label = "┌─ " + lang + " ";
        const size_t width = terminal_width();
        const size_t fill = width > display_width(label) ? width - display_width(label) : 0;
        std::string out = "\033[36m" + label + repeat("─", fill) + kAnsiReset + "\n";
        for (const std::string& line : block.contents) {
            out += "\033[33m│ \033[0m" + line + "\n";
        }
        out += "\033[36m└" + repeat("─", width > 1 ? width - 1 : 0) + kAnsiReset + "\n";
        return out;
    }

    static std::string normalize_lang(const std::string& lang)
    {
        std::string out = lang.empty() ? "code" : lang;
        if (out == "cpp" || out == "c++") out = "C++";
        return out;
    }

    std::string drain(bool is_end)
    {
        std::string out;
        size_t pos = 0;
        while ((pos = m_buffer.find('\n')) != std::string::npos) {
            std::string line = m_buffer.substr(0, pos);
            m_buffer.erase(0, pos + 1);
            out += take_stream_line(line);
        }
        if (is_end) {
            if (!m_buffer.empty()) {
                std::string line = m_buffer;
                m_buffer.clear();
                out += take_stream_line(line);
            }
            out += flush_table();
        }
        return out;
    }

    // Need all rows before computing column widths.
    std::string take_stream_line(const std::string& raw)
    {
        if (!m_in_code && is_table_row(trim(rtrim(raw)))) {
            m_table.push_back(trim(rtrim(raw)));
            return "";
        }
        return flush_table() + render_stream_line(raw) + "\n";
    }

    std::string flush_table()
    {
        if (m_table.empty()) return "";
        Block block{};
        block.type = BlockType::Table;
        block.contents = m_table;
        m_table.clear();
        return render_table(block);
    }

    std::string render_stream_line(const std::string& raw)
    {
        const std::string line = rtrim(raw);
        const std::string text = trim(line);

        if (is_code_fence(text)) {
            m_in_code = !m_in_code;
            const std::string label = "┌─ " + normalize_lang(trim(text.substr(3))) + " ";
            const size_t width = terminal_width();
            if (m_in_code) {
                const size_t fill = width > display_width(label) ? width - display_width(label) : 0;
                return "\033[36m" + label + repeat("─", fill) + kAnsiReset;
            }
            return "\033[36m└" + repeat("─", width > 1 ? width - 1 : 0) + kAnsiReset;
        }
        if (m_in_code) {
            return "\033[33m│ \033[0m" + line;
        }

        int level = heading_level(line);
        if (level > 0) {
            std::string style = heading_style(level);
            if (level <= 2) style += "\033[4m";
            return style + render_inline(trim(line.substr(line.find_first_not_of(" #")))) + kAnsiReset;
        }
        if (!text.empty() && text[0] == '>') {
            return "\033[36m┃ \033[0m" + render_inline(render_math(trim(text.substr(1))));
        }
        int indent = 0;
        std::string marker{};
        size_t body = 0;
        if (list_marker(line, indent, marker, body)) {
            return std::string(indent * 2, ' ') + "\033[32m" + marker + "\033[0m "
                + render_inline(render_math(trim(line.substr(body))));
        }
        if (is_rule(text)) {
            return "\033[90m" + repeat("─", terminal_width()) + "\033[0m";
        }
        return render_inline(render_math(text));
    }

    std::vector<std::string> list_lines(const std::string& content) const
    {
        std::vector<std::string> lines;
        std::istringstream stream(content);
        std::string line;
        while (std::getline(stream, line)) {
            lines.push_back(line);
        }
        return lines;
    }

    static std::string get_super(char c)
    {
        for (const auto& t : SUPER_SUB_SCRIPT) {
            if (std::get<0>(t) == tolower(c)) return std::get<1>(t);
        }
        return std::string(1, c);
    }

    static std::string get_sub(char c)
    {
        for (const auto& t : SUPER_SUB_SCRIPT) {
            if (std::get<0>(t) == tolower(c)) return std::get<2>(t);
        }
        return std::string(1, c);
    }

    // LaTeX to Unicode (terminal-friendly).
    static std::string improved_formula(const std::string& latex)
    {
        std::string parsed;
        for (size_t i = 0; i < latex.length(); ++i) {
            bool cmd_matched = false;
            for (const auto& sym : SYMBOL_MAP) {
                if (latex.compare(i, sym.first.length(), sym.first) == 0) {
                    parsed += sym.second;
                    i += sym.first.length() - 1;
                    cmd_matched = true;
                    break;
                }
            }
            if (cmd_matched) continue;

            if (latex[i] == '^' || latex[i] == '_') {
                const bool is_super = (latex[i] == '^');
                std::string content;
                char wrap_char = 0;
                if (i + 1 < latex.length() && latex[++i] == '{') {
                    wrap_char = '}';
                    ++i;
                }
                while (i < latex.length()) {
                    if (wrap_char && latex[i] == wrap_char) break;
                    if (!wrap_char && (latex[i] == ' ' || latex[i] == '_' || latex[i] == '^')) break;
                    content += latex[i++];
                }
                if (content.empty()) {
                    parsed += is_super ? '^' : '_';
                    continue;
                }
                const bool needs_wrap = content.length() > 1 ||
                    !isalnum(static_cast<unsigned char>(content[0]));
                std::string text = needs_wrap ? (is_super ? "⁽" : "₍") : "";
                for (char c : content) {
                    text += is_super ? get_super(c) : get_sub(c);
                }
                parsed += text + (needs_wrap ? (is_super ? "⁾" : "₎") : "");
                continue;
            }

            if (i + 4 < latex.length() && latex.compare(i, 5, "\\frac") == 0) {
                i += 5;
                std::string num = read_group(latex, i);
                std::string den = read_group(latex, i);
                parsed += wrap_if_needed(improved_formula(num)) + "/" + wrap_if_needed(improved_formula(den));
                continue;
            }
            parsed += latex[i];
        }
        return parsed;
    }

    static std::string read_group(const std::string& latex, size_t& i)
    {
        if (i >= latex.length()) return "";
        if (latex[i] != '{') return std::string(1, latex[i++]);
        ++i;
        std::string out;
        int depth = 0;
        while (i < latex.length()) {
            if (latex[i] == '{') ++depth;
            else if (latex[i] == '}') {
                if (depth == 0) {
                    ++i;
                    break;
                }
                --depth;
            }
            out += latex[i++];
        }
        return out;
    }

    static std::string wrap_if_needed(const std::string& text)
    {
        if (text.empty()) return text;
        const bool needs_wrap = text.length() > 1 ||
            (!isalnum(static_cast<unsigned char>(text[0])) && text != "π" && text != "Σ");
        return needs_wrap ? "(" + text + ")" : text;
    }

    // $...$ and $$...$$ to Unicode; escaped \$ stays literal.
    static std::string render_math(const std::string& text)
    {
        std::string out;
        size_t i = 0;
        while (i < text.size()) {
            if (text[i] == '\\' && i + 1 < text.size() && text[i + 1] == '$') {
                out += '$';
                i += 2;
                continue;
            }
            if (text[i] == '$') {
                const bool block = text.compare(i, 2, "$$") == 0;
                const std::string delim = block ? "$$" : "$";
                size_t end = text.find(delim, i + delim.size());
                if (end == std::string::npos) {
                    out += text[i++];
                    continue;
                }
                std::string body = improved_formula(text.substr(i + delim.size(), end - i - delim.size()));
                out += block ? ("\n" + body + "\n") : body;
                i = end + delim.size();
                continue;
            }
            out += text[i++];
        }
        return out;
    }
};

class Configs {
private:
    std::string m_filename{};
    std::string m_content{};
private:
    std::string getFileVariable(const std::string& content, const std::string& keyword)
    {
        std::string val = {};
        size_t pos = content.find(keyword);
        if (pos != std::string::npos) {
            val = content.substr(pos, content.size());
            pos = val.find("=");
            size_t org = val.find("&");
            if (org == std::string::npos) {
                val = val.substr(pos + 1, val.size() - pos - 1);
            } else {
                val = val.substr(pos + 1, org - pos - 1);
            }
        }
        if (!val.empty() && val.back() == '\n') {
            val.pop_back();
        }
        return val;
    }
public:
    static Configs& getConfig()
    {
        static Configs config("params.txt");
        return config;
    }
    explicit Configs(const std::string& filename) : m_filename(filename) { }
    ~Configs() = default;
    std::string getVariable(const std::string& keyword)
    {
        if (m_content.empty()) {
            m_content = getFileContent(m_filename);
        }
        if (m_content.empty()) {
            if (keyword == "model") {
                return "0";
            } else
                return {};
        }
        std::string val = getFileVariable(m_content, keyword);
        if (keyword == "model") {
            val = (std::stoi(val) > 4 ? "4" : val);
        }
        return val;
    }
};
