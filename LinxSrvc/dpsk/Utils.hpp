// by: DeepSeek/GPT-4o
#include <string>
#include <vector>
#include <sstream>
#include <numeric>
#include <regex>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

// using namespace std::string_literals; // C++14 or later for "s" suffix

enum class BlockType {
    None,
    Heading,
    List,
    Paragraph,
    CodeBlock
};

struct Block {
    BlockType type;
    int heading_level;
    std::vector<std::string> contents;
    std::string language;
};

// Unicode mathematical symbol mapping
const std::vector<std::pair<std::string, std::string>> SYMBOL_MAP = {
    {"\\int", "∫"}, {"\\infty", "∞"}, {"\\pi", "π"},
    {"\\sqrt", "√"}, {"\\frac", "/"}, {"\\sum", "Σ"}
};

// Superscript and subscript conversion table (including Greek letters)
const std::vector<std::tuple<char, std::string, std::string>> SUPER_SUB_SCRIPT = {
    {'0', "⁰", "₀"}, {'1', "¹", "₁"}, {'2', "²", "₂"}, {'3', "³", "₃"},
    {'4', "⁴", "₄"}, {'5', "⁵", "₅"}, {'6', "⁶", "₆"}, {'7', "⁷", "₇"},
    {'8', "⁸", "₈"}, {'9', "⁹", "₉"}, {'-', "⁻", "₋"}, {'+', "⁺", "₊"},
    {'a', "ᵃ", "ₐ"}, {'b', "ᵇ", "♭"}, {'c', "ᶜ", "꜀"}, {'d', "ᵈ", "ꝱ"},
    {'e', "ᵉ", "ₑ"}, {'f', "ᶠ", "ꞙ"}, {'x', "ˣ", "ₓ"}, {'y', "ʸ", "ᵧ"}
};

// Formula location structure
struct FormulaSegment {
    size_t start;
    size_t tail;
    bool is_block;
};

class Markdown {
public:
    std::vector<Block> parse_blocks(const std::string& content) const
    {
        try {
            std::vector<std::string> lines = list_lines(content);
            std::vector<Block> blocks;
            Block using_block{ BlockType::None, 0, {} };
            bool in_code_block = false;

            for (const std::string& line : lines) {
                if (is_code_block_delimiter(line)) {
                    handle_code_block_delimiter(line, using_block, blocks, in_code_block);
                    continue;
                }

                if (in_code_block) {
                    using_block.contents.push_back(line);
                    continue;
                }

                if (line.empty()) {
                    handle_empty_line(using_block, blocks);
                    continue;
                }

                int heading_level = is_heading_line(line);
                if (heading_level > 0) {
                    handle_heading_line(line, heading_level, using_block, blocks);
                    continue;
                }

                if (is_list_item(line)) {
                    handle_list_item(line, using_block, blocks);
                    continue;
                }

                handle_paragraph(line, using_block, blocks);
            }

            if (using_block.type != BlockType::None) {
                blocks.push_back(using_block);
            }

            return blocks;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing content: " << e.what() << std::endl;
            return {};
        }
    }

    static std::string Parse(const std::string& content)
    {
        Markdown text;
        std::vector<Block> blocks = text.parse_blocks(parse_formulas(easy_formulas(content)));
        std::stringstream parsed;
        for (const Block& block : blocks) {
            switch (block.type) {
            case BlockType::Heading:
                switch (block.heading_level) {
                case 1:
                    parsed << "\033[1;31m" << block.contents[0] << "\033[0m" << std::endl;
                    break;
                case 2:
                    parsed << "\033[1;32m" << block.contents[0] << "\033[0m" << std::endl;
                    break;
                case 3:
                    parsed << "\033[1;33m" << block.contents[0] << "\033[0m" << std::endl;
                    break;
                case 4:
                    parsed << "\033[1;34m" << block.contents[0] << "\033[0m" << std::endl;
                    break;
                case 5:
                    parsed << "\033[1;35m" << block.contents[0] << "\033[0m" << std::endl;
                    break;
                case 6:
                    parsed << "\033[1;36m" << block.contents[0] << "\033[0m" << std::endl;
                    break;
                default:
                    parsed << "\033[1;37m" << block.contents[0] << "\033[0m" << std::endl;
                    break;
                }
                break;
            case BlockType::List:
                for (const std::string& item : block.contents) {
                    parsed << "\033[1;32m* " << item << "\033[0m" << std::endl;
                }
                break;
            case BlockType::Paragraph:
                parsed << "\033[0;37m" << std::accumulate(std::next(block.contents.begin()), block.contents.end(), block.contents[0], [](std::string a, const std::string& b) { return std::move(a) + " " + b; }) << "\033[0m" << std::endl;
                break;
            case BlockType::CodeBlock:
                if (block.language == "cpp") {
                    parsed << "\033[1;36m+------C++-------+\033[0m\n" << std::endl;
                } else {
                    parsed << "\033[1;36m+------" << block.language << "-------+\033[0m\n" << std::endl;
                }
                for (const std::string& line : block.contents) {
                    parsed << "\033[0;33m" << line << "\033[0m" << std::endl;
                }
                parsed << "\033[1;36m\n+-------------";
                for (size_t i = 0; i < block.language.size(); i++) {
                    parsed << "-";
                }
                parsed << "+\033[0m" << std::endl;
                break;
            default:
                break;
            }
            parsed << std::endl;
        }
        return parsed.str();
    }

private:
    void handle_code_block_delimiter(const std::string& line, Block& using_block, std::vector<Block>& blocks, bool& in_code_block) const
    {
        if (in_code_block) {
            blocks.push_back(using_block);
            using_block = { BlockType::None, 0, {} };
        } else {
            if (using_block.type != BlockType::None) {
                blocks.push_back(using_block);
                using_block.contents.clear();
            }
            using_block.type = BlockType::CodeBlock;
        }
        using_block.language = line.substr(3);
        in_code_block = !in_code_block;
    }

    void handle_empty_line(Block& using_block, std::vector<Block>& blocks) const
    {
        if (using_block.type != BlockType::None) {
            blocks.push_back(using_block);
            using_block = { BlockType::None, 0, {} };
        }
    }

    void handle_heading_line(const std::string& line, int heading_level, Block& using_block, std::vector<Block>& blocks) const
    {
        if (using_block.type != BlockType::None) {
            blocks.push_back(using_block);
            using_block.contents.clear();
        }
        using_block = { BlockType::Heading, heading_level, {trim(line.substr(line.find_first_not_of(" #")))} };
        blocks.push_back(using_block);
        using_block = { BlockType::None, 0, {} };
    }

    void handle_list_item(const std::string& line, Block& using_block, std::vector<Block>& blocks) const
    {
        if (using_block.type != BlockType::List) {
            if (using_block.type != BlockType::None) {
                blocks.push_back(using_block);
                using_block.contents.clear();
            }
            using_block.type = BlockType::List;
        }
        using_block.contents.push_back(parse_inline_elements(trim(line.substr(line.find(' ', line.find_first_not_of(' ')) + 1))));
    }

    void handle_paragraph(const std::string& line, Block& using_block, std::vector<Block>& blocks) const
    {
        if (using_block.type != BlockType::Paragraph) {
            if (using_block.type != BlockType::None) {
                blocks.push_back(using_block);
                using_block.contents.clear();
            }
            using_block.type = BlockType::Paragraph;
        }
        using_block.contents.push_back(parse_inline_elements(trim(line)));
    }

    int is_heading_line(const std::string& line) const
    {
        size_t pos = line.find_first_not_of(' ');
        if (pos == std::string::npos || line[pos] != '#') return 0;

        int level = 0;
        while (pos < line.size() && line[pos] == '#' && level < 6) {
            level++;
            pos++;
        }
        return (pos < line.size() && line[pos] == ' ') ? level : 0;
    }

    bool is_list_item(const std::string& line) const
    {
        size_t pos = line.find_first_not_of(' ');
        if (pos == std::string::npos) return false;
        char c = line[pos];
        return (c == '*' || c == '-' || c == '+') && (pos + 1 < line.size() && line[pos + 1] == ' ');
    }

    bool is_code_block_delimiter(const std::string& line) const
    {
        return (line.find("```") == 0);
    }

    std::string trim(const std::string& s) const
    {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    std::string parse_inline_elements(const std::string& line) const
    {
        std::string parsed = line;

        // Parse bold (** or __)
        static const std::regex bold_regex("\\*\\*(.*?)\\*\\*|__(.*?)__");
        // Parse italic (* or _)
        static const std::regex italic_regex("\\*(.*?)\\*|_(.*?)_");
        // Parse links ([text](url))
        static const std::regex link_regex("\\[(.*?)\\]\\((.*?)\\)");
        // Parse inline code (`code`)
        static const std::regex code_regex("`(.*?)`");

        parsed = std::regex_replace(parsed, bold_regex, "\033[1m$1$2\033[0m");
        parsed = std::regex_replace(parsed, italic_regex, "\033[3m$1$2\033[0m");
        parsed = std::regex_replace(parsed, link_regex, "\033[34m$1\033[0m(\033[4m$2\033[0m)");
        parsed = std::regex_replace(parsed, code_regex, "\033[32m$1\033[0m");

        return parsed;
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

    static std::string easy_formulas(const std::string& mk)
    {
        std::string parsed = mk;
        // 1. Escape any backslashes and $ to avoid conflicts during regex processing
        parsed = std::regex_replace(
            parsed,
            std::regex(R"((?:\$([\\$])))"),
            "\x01$1"
        );
        // 2. Handle display math (block-level) using $$...$$
        parsed = std::regex_replace(
            parsed,
            std::regex(R"((\$\$)([^\$]*(?:\$(?!\$)[^\$]*)*)(\$\$))"),
            "<display-math>$2</display-math>"
        );
        // 3. Handle inline math (inline) using $...$
        parsed = std::regex_replace(
            parsed,
            std::regex(R"((\$)([^\\$]*(?:\\.[^\\$]*)*)(\$))"),
            "<inline-math>$2</inline-math>"
        );
        // 4. Restore escaped characters
        parsed = std::regex_replace(
            parsed,
            std::regex(R"(\x01([\\$]))"),
            "\\$1"
        );
        return parsed;
    }

    static std::string get_super(char c)
    {
        for (const auto& t : SUPER_SUB_SCRIPT)
            if (std::get<0>(t) == tolower(c)) return std::get<1>(t);
        return std::string(1, c);
    }

    static std::string get_sub(char c)
    {
        for (const auto& t : SUPER_SUB_SCRIPT)
            if (std::get<0>(t) == tolower(c)) return std::get<2>(t);
        return std::string(1, c);
    }

    // Enhanced formula conversion
    static std::string improved_formula(const std::string& latex)
    {
        std::string parsed;

        for (size_t i = 0; i < latex.length(); ++i) {
            // Handle LaTeX commands
            bool cmd_matched = false;
            for (const auto& sym : SYMBOL_MAP) {
                if (latex.substr(i, sym.first.length()) == sym.first) {
                    parsed += sym.second;
                    i += sym.first.length() - 1;
                    cmd_matched = true;
                    break;
                }
            }
            if (cmd_matched) continue;

            // Handle superscript and subscript
            if (latex[i] == '^' || latex[i] == '_') {
                bool is_super = (latex[i] == '^');
                std::string content;
                char wrap_char = 0;

                if (latex[++i] == '{') {
                    wrap_char = '}';
                    i++;
                }

                while (i < latex.length()) {
                    if (wrap_char && latex[i] == wrap_char) break;
                    if (!wrap_char && (latex[i] == ' ' || latex[i] == '_' || latex[i] == '^')) break;
                    content += latex[i++];
                }

                // Add parentheses to wrap complex content
                bool needs_wrap = content.length() > 1 || !isalnum(content[0]);
                std::string prefix = needs_wrap ? (is_super ? "⁽" : "₍") : "";
                std::string suffix = needs_wrap ? (is_super ? "⁾" : "₎") : "";

                for (char c : content) {
                    prefix += is_super ? get_super(c) : get_sub(c);
                }
                parsed += prefix + suffix;

                continue;

                // Handle fractions
                if (i + 4 < latex.length() && latex.substr(i, 5) == "\\frac") {
                    i += 5;
                    std::string num, den;
                    int brace_count = 0;

                    // Parse numerator
                    if (latex[i] == '{') {
                        i++;
                        while (i < latex.length() && (latex[i] != '}' || brace_count > 0)) {
                            if (latex[i] == '{') brace_count++;
                            if (latex[i] == '}') brace_count--;
                            num += latex[i++];
                        }
                        i++;
                    }
                    // Parse denominator
                    if (latex[i] == '{') {
                        i++;
                        while (i < latex.length() && (latex[i] != '}' || brace_count > 0)) {
                            if (latex[i] == '{') brace_count++;
                            if (latex[i] == '}') brace_count--;
                            den += latex[i++];
                        }
                        i++;
                    }

                    parsed += "(" + num + ")/(" + den + ")";
                    continue;
                }

                // Default character handling
                parsed += latex[i];
            }
            // Enhanced fraction handling
            if (i + 4 < latex.length() && latex.substr(i, 5) == "\\frac") {
                i += 5;
                std::string num, den;
                int brace_count = 0;

                // Parse numerator
                if (latex[i] == '{') {
                    i++;
                    while (i < latex.length() && (latex[i] != '}' || brace_count > 0)) {
                        if (latex[i] == '{') brace_count++;
                        if (latex[i] == '}') brace_count--;
                        num += latex[i++];
                    }
                    i++;
                } else {
                    num = latex[i++];
                }

                // Parse denominator
                if (latex[i] == '{') {
                    i++;
                    while (i < latex.length() && (latex[i] != '}' || brace_count > 0)) {
                        if (latex[i] == '{') brace_count++;
                        if (latex[i] == '}') brace_count--;
                        den += latex[i++];
                    }
                    i++;
                } else {
                    den = latex[i++];
                }

                // Recursively process numerator and denominator content
                std::string conv_num = improved_formula(num);
                std::string conv_den = improved_formula(den);

                // Intelligent parenthesis addition rules
                auto needs_wrap = [](const std::string& s) {
                    return s.length() > 1 ||
                        (!isalnum(s[0]) && s != "π" && s != "Σ"); // Special handling for common symbols
                    };

                std::string display_num = needs_wrap(conv_num) ? "(" + conv_num + ")" : conv_num;
                std::string display_den = needs_wrap(conv_den) ? "(" + conv_den + ")" : conv_den;

                parsed += display_num + "/" + display_den;
                continue;
            }
        }
        return parsed;
    }

    static std::vector<FormulaSegment> locate_formulas(const std::string& content)
    {
        std::vector<FormulaSegment> formulas;
        size_t pos = 0;

        while (pos < content.length()) {
            size_t block_start = content.find("$$", pos);
            size_t inline_start = content.find('$', pos);

            // 优先处理块公式
            if (block_start != std::string::npos &&
                (inline_start == std::string::npos || block_start < inline_start)) {
                size_t block_end = content.find("$$", block_start + 2);
                if (block_end == std::string::npos) break;

                formulas.push_back({ block_start, block_end + 1, true });
                pos = block_end + 2;
            }
            // Process inline formulas
            else if (inline_start != std::string::npos) {
                size_t inline_end = content.find('$', inline_start + 1);
                if (inline_end == std::string::npos) break;

                formulas.push_back({ inline_start, inline_end, false });
                pos = inline_end + 1;
            } else {
                break;
            }
        }

        // Sort by start position in descending order
        std::sort(formulas.begin(), formulas.end(), [](const FormulaSegment& a, const FormulaSegment& b) {
            return a.start > b.start;
            });

        return formulas;
    }

    // Main processing function
    static std::string parse_formulas(const std::string& markdown)
    {
        std::string output = markdown;
        auto formulas = locate_formulas(markdown);

        for (const auto& seg : formulas) {
            std::string original = output.substr(seg.start, seg.tail - seg.start + (seg.is_block ? 1 : 0));
            std::string content = original.substr(
                seg.is_block ? 2 : 1,
                original.length() - (seg.is_block ? 4 : 2)
            );

            std::string converted = improved_formula(content);
            output.replace(seg.start, seg.tail - seg.start + 1, converted);
        }

        return output;
    }
};

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
        if (val.back() == '\n') {
            val.pop_back();
        }
        return val;
    }
public:
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
            return {};
        }
        std::string val = getFileVariable(m_content, keyword);
        if (keyword == "model") {
            val = (std::stoi(val) > 4 ? "4" : val);
        }
        return val;
    }
};
