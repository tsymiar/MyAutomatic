// by: deepseek
#include <string>
#include <regex>

// ANSI escape codes for text styling
const std::string BOLD = "\033[1m";
const std::string ITALIC = "\033[3m";
const std::string CODE = "\033[2m\033[47m";
const std::string LINK = "\033[34m\033[4m";
const std::string RESET = "\033[0m";
const std::string HEADER_COLOR = "\033[36m";

class Markdown {
private:
    // Precompiled regex patterns
    static const std::regex CODE_BLOCK_START;
    static const std::regex HEADER_PATTERN;
    static const std::regex LIST_PATTERN;
    static const std::regex BOLD_PATTERN;
    static const std::regex ITALIC_PATTERN;
    static const std::regex INLINE_CODE_PATTERN;
    static const std::regex LINK_PATTERN;

    static std::string formatCodeBlockHeader(const std::string& lang)
    {
        const std::string base = HEADER_COLOR + "+----- Block";
        if (lang.empty()) {
            return base + " ----+" + RESET;
        }
        return base + " (" + lang + ") ---+" + RESET;
    }

public:
    static std::string Parse(const std::string& line)
    {
        static bool in_code_block = false;
        static std::string code_block_lang;

        // Code block handling
        if (std::regex_match(line, CODE_BLOCK_START)) {
            in_code_block = !in_code_block;
            if (in_code_block) {
                code_block_lang = line.substr(3);
                return formatCodeBlockHeader(code_block_lang);
            }
            code_block_lang.clear();
            return HEADER_COLOR + "+-------------------+" + RESET;
        }
        if (in_code_block) return line;

        // Header handling
        std::smatch match;
        if (std::regex_search(line, match, HEADER_PATTERN)) {
            if (match.size() >= 3) {
                const int level = match[1].length();
                const std::string text = match[2];
                const char underline_char = (level == 1) ? '=' : '-';
                return HEADER_COLOR + BOLD + text + RESET + "\n"
                    + HEADER_COLOR + std::string(text.length(), underline_char) + RESET;
            }
        }

        // List handling
        if (std::regex_search(line, match, LIST_PATTERN)) {
            if (match.size() >= 2) {
                return "* " + match[2].str();
            }
        }

        // Text styling transformations
        std::string parsed = line;
        parsed = std::regex_replace(parsed, BOLD_PATTERN, BOLD + "$1" + RESET);
        parsed = std::regex_replace(parsed, ITALIC_PATTERN, ITALIC + "$1" + RESET);
        parsed = std::regex_replace(parsed, INLINE_CODE_PATTERN, CODE + "$1" + RESET);
        parsed = std::regex_replace(parsed, LINK_PATTERN, "$1 " + LINK + "($2)" + RESET);

        return parsed;
    }
};

// Precompiled regex patterns
const std::regex Markdown::CODE_BLOCK_START("^```[a-zA-Z0-9]*$");
const std::regex Markdown::HEADER_PATTERN("^(#{1,6})\\s+(.*)$");
const std::regex Markdown::LIST_PATTERN("^([*+-])\\s+(.*)$");
const std::regex Markdown::BOLD_PATTERN("\\*\\*(.*?)\\*\\*");
const std::regex Markdown::ITALIC_PATTERN("\\*(.*?)\\*");
const std::regex Markdown::INLINE_CODE_PATTERN("`(.*?)`");
const std::regex Markdown::LINK_PATTERN("\\[(.*?)\\]\\((.*?)\\)");
