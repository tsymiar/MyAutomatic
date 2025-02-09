// by: DeepSeek
#include <string>
#include <regex>

// ANSI escape codes for text styling
const std::string BOLD = "\033[1m";
const std::string ITALIC = "\033[3m";
const std::string CODE = ""; // "\033[2m\033[47m";
const std::string LINK = "\033[34m\033[4m";
const std::string RESET = "\033[0m";
const std::string HEADER_COLOR = "\033[36m";
const std::string QUOTE_COLOR = "\033[33m";
const std::string HR_COLOR = "\033[37m";

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
    static const std::regex QUOTE_PATTERN;
    static const std::regex HR_PATTERN;
    static const std::regex IMAGE_PATTERN;

    static std::string formatBlockHeader(const std::string& lang)
    {
        constexpr int base_length = 12; // "+----- Block ".length()
        const std::string base = HEADER_COLOR + "+----- Block";
        if (lang.empty()) {
            return base + " -----+" + RESET;
        }
        return base + " (" + lang + ") --+" + RESET;
    }

    static std::string applyTextStyling(const std::string& text)
    {
        std::string parsed = text;
        parsed = std::regex_replace(parsed, BOLD_PATTERN, BOLD + "$1" + RESET);
        parsed = std::regex_replace(parsed, ITALIC_PATTERN, ITALIC + "$1" + RESET);
        parsed = std::regex_replace(parsed, INLINE_CODE_PATTERN, CODE + "$1" + RESET);
        parsed = std::regex_replace(parsed, LINK_PATTERN, "$1 " + LINK + "($2)" + RESET);
        return parsed;
    }

public:
    static std::string Parse(const std::string& line)
    {
        struct ParseState {
            bool in_code_block = false;
            std::string code_block_lang;
        };
        static ParseState state;

        // Code block handling
        if (std::regex_match(line, CODE_BLOCK_START)) {
            state.in_code_block = !state.in_code_block;
            if (state.in_code_block) {
                state.code_block_lang = line.substr(3);
                return formatBlockHeader(state.code_block_lang);
            }
            const std::string result = HEADER_COLOR + "+-------------------+" + RESET;
            state.code_block_lang.clear();
            return result;
        }
        if (state.in_code_block) return "│ " + line;

        // Header handling
        std::smatch header_match;
        if (std::regex_match(line, header_match, HEADER_PATTERN)) {
            if (header_match.size() != 3) return line;
            const int level = header_match[1].length();
            const std::string text = header_match[2];
            if (text.empty()) return line;
            const char underline_char = (level == 1) ? '=' : '-';
            return HEADER_COLOR + BOLD + text + RESET + "\n" +
                HEADER_COLOR + std::string(text.length(), underline_char) + RESET;
        }

        // List handling
        std::smatch list_match;
        if (std::regex_match(line, list_match, LIST_PATTERN)) {
            if (list_match.size() != 3) return line;
            return "  • " + list_match[2].str();
        }

        // Quote handling
        std::smatch quote_match;
        if (std::regex_match(line, quote_match, QUOTE_PATTERN)) {
            return QUOTE_COLOR + "> " + quote_match[1].str() + RESET;
        }

        // Horizontal rule handling
        if (std::regex_match(line, HR_PATTERN)) {
            return HR_COLOR + "--------------------" + RESET;
        }

        // Image handling
        std::smatch image_match;
        if (std::regex_match(line, image_match, IMAGE_PATTERN)) {
            return "![Image: " + image_match[1].str() + "](" + image_match[2].str() + ")";
        }

        // Text styling transformations
        return applyTextStyling(line);
    }
};

// static regex patterns with explicit ECMAScript grammar
const std::regex Markdown::CODE_BLOCK_START(R"(^```[a-zA-Z0-9]*\s*$)");
const std::regex Markdown::HEADER_PATTERN(R"(^#{1,6}\s+(\S[^\n]*?\S)\s*$)");
const std::regex Markdown::LIST_PATTERN(R"(^\s*[*+-]\s+(.*)$)");
const std::regex Markdown::BOLD_PATTERN(R"(\*\*(\S(?:.*?\S)?)\*\*)");
const std::regex Markdown::ITALIC_PATTERN(R"(\*(\S(?:.*?\S)?)\*(?!\*))");
const std::regex Markdown::INLINE_CODE_PATTERN(R"(`([^`]+)`)");
const std::regex Markdown::LINK_PATTERN(R"(\[([^\]]+)\]\(\s*(\S+)\s*\))");
const std::regex Markdown::QUOTE_PATTERN(R"(^>\s+(.*)$)");
const std::regex Markdown::HR_PATTERN(R"(^\s*([-*_])(?:\s*\1\s*){2,}$)");
const std::regex Markdown::IMAGE_PATTERN(R"(!\[([^\]]+)\]\(\s*(\S+)\s*\))");
