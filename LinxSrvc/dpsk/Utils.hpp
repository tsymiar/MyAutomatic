// by: DeepSeek/GPT-4o
#include <string>
#include <vector>
#include <sstream>
#include <numeric>
#include <regex>
#include <fstream>
#include <iostream>

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
                    continue;
                }

                if (in_code_block) {
                    using_block.contents.push_back(line);
                    continue;
                }

                if (line.empty()) {
                    if (using_block.type != BlockType::None) {
                        blocks.push_back(using_block);
                        using_block = { BlockType::None, 0, {} };
                    }
                    continue;
                }

                int heading_level = is_heading_line(line);
                if (heading_level > 0) {
                    if (using_block.type != BlockType::None) {
                        blocks.push_back(using_block);
                        using_block.contents.clear();
                    }
                    using_block = { BlockType::Heading, heading_level, {trim(line.substr(line.find_first_not_of(" #")))} };
                    blocks.push_back(using_block);
                    using_block = { BlockType::None, 0, {} };
                    continue;
                }

                if (is_list_item(line)) {
                    if (using_block.type != BlockType::List) {
                        if (using_block.type != BlockType::None) {
                            blocks.push_back(using_block);
                            using_block.contents.clear();
                        }
                        using_block.type = BlockType::List;
                    }
                    using_block.contents.push_back(parse_inline_elements(trim(line.substr(line.find(' ', line.find_first_not_of(' ')) + 1))));
                    continue;
                }

                if (using_block.type != BlockType::Paragraph) {
                    if (using_block.type != BlockType::None) {
                        blocks.push_back(using_block);
                        using_block.contents.clear();
                    }
                    using_block.type = BlockType::Paragraph;
                }
                using_block.contents.push_back(parse_inline_elements(trim(line)));
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
        Markdown mk;
        std::vector<Block> blocks = mk.parse_blocks(parse_formulas(content));
        std::stringstream parsed;
        for (const Block& block : blocks) {
            switch (block.type) {
            case BlockType::Heading:
                parsed << "H" << block.heading_level << ": " << block.contents[0] << std::endl;
                break;
            case BlockType::List:
                for (const std::string& item : block.contents) {
                    parsed << "* " << item << std::endl;
                }
                break;
            case BlockType::Paragraph:
                parsed << std::accumulate(std::next(block.contents.begin()), block.contents.end(), block.contents[0], [](std::string a, const std::string& b) { return std::move(a) + " " + b; }) << std::endl;
                break;
            case BlockType::CodeBlock:
                if (block.language == "cpp") {
                    parsed << "+------C++-------+\n" << std::endl;
                } else {
                    parsed << "+------" << block.language << "-------+\n" << std::endl;
                }
                for (const std::string& line : block.contents) {
                    parsed << line << std::endl;
                }
                parsed << "\n+-------------";
                for (size_t i = 0; i < block.language.size(); i++) {
                    parsed << "-";
                }
                parsed << "+" << std::endl;
                break;
            default:
                break;
            }
            parsed << std::endl;
        }
        return parsed.str();
    }

private:
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

    static std::string parse_formulas(const std::string& mk)
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
};

class Config {
private:
    std::string m_filename{};
private:
    std::string getFileContent(const std::string& filename)
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
    std::string getFileVariable(const std::string& filename, const std::string& keyword)
    {
        std::string content = getFileContent(filename);
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
    Config(const std::string& filename) : m_filename(filename) { }
    std::string getVariable(const std::string& keyword)
    {
        return getFileVariable(m_filename, keyword);
    }
};
