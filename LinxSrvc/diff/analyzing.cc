#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <filesystem>

namespace fs = std::filesystem;

constexpr const char* RED = "\033[31m";
constexpr const char* GREEN = "\033[32m";
constexpr const char* MAGENTA = "\033[35m";
constexpr const char* CYAN = "\033[36m";
constexpr const char* YELLOW = "\033[33m";
constexpr const char* RESET = "\033[0m";

struct FileResult {
    double ratio = 0.0;
    size_t duplicates = 0;
    size_t total1 = 0;
    size_t total2 = 0;
    std::vector<std::string> diff{};
    std::string filename{};
};

class FileAnalyzer {
    bool use_color_ = false;
    bool show_diff_ = false;
    std::vector<fs::path> exclude_paths_{};

    static bool is_text_file(const fs::path& path)
    {
        static const std::unordered_set<std::string> TEXT_EXTS = {
            ".cpp", ".h", ".hpp", ".c", ".cc", ".cxx", ".py",
            ".java", ".txt", ".md", ".json", ".xml", ".yml", ".yaml",
            ".cmake", ".toml", ".ini", ".cfg", ".log", ".sh", ".bat",
            ".html", ".css", ".js", ".ts"
        };
        std::string ext = path.extension().string();
        for (auto& c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        return TEXT_EXTS.count(ext) > 0;
    }

    static std::vector<std::string> read_lines(const fs::path& path)
    {
        std::vector<std::string> lines;
        if (!is_text_file(path)) return lines;

        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty()) {
                lines.push_back(line);
            }
        }
        return lines;
    }

    // Extract effective content: strip comments + whitespace in single pass
    static std::string get_effective_content(const std::string& line)
    {
        std::string content;
        content.reserve(line.size());
        bool in_block_comment = false;
        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (in_block_comment) {
                if (c == '*' && i + 1 < line.size() && line[i + 1] == '/') {
                    in_block_comment = false;
                    i++; // skip closing '/'
                }
                continue;
            }
            if (c == '/' && i + 1 < line.size()) {
                if (line[i + 1] == '*') {
                    in_block_comment = true;
                    i++; // skip '*'
                    continue;
                }
                if (line[i + 1] == '/') break; // line comment → discard rest
            }
            if (!std::isspace(static_cast<unsigned char>(c))) {
                content.push_back(c);
            }
        }
        return content;
    }

    std::string colorize(const std::string& text, const std::string& color) const
    {
        return use_color_ ? (color + text + RESET) : text;
    }

    bool is_excluded(const fs::path& path) const
    {
        for (const auto& exclude_path : exclude_paths_) {
            if (fs::equivalent(path, exclude_path) || fs::equivalent(path.parent_path(), exclude_path)) {
                return true;
            }
        }
        return false;
    }
public:
    FileAnalyzer(bool use_color, bool show_diff, const std::vector<fs::path>& exclude_paths = {})
        : use_color_(use_color), show_diff_(show_diff), exclude_paths_(exclude_paths)
    {}

    FileResult compare_files(const fs::path& file1, const fs::path& file2) const
    {
        FileResult out;
        const auto lines1 = read_lines(file1);
        const auto lines2 = read_lines(file2);

        // Preprocessing: Extract effective content
        std::vector<std::string> preprocessed_lines1;
        preprocessed_lines1.reserve(lines1.size());
        for (const auto& line : lines1) {
            preprocessed_lines1.push_back(get_effective_content(line));
        }

        std::vector<std::string> preprocessed_lines2;
        preprocessed_lines2.reserve(lines2.size());
        for (const auto& line : lines2) {
            preprocessed_lines2.push_back(get_effective_content(line));
        }

        // Calculate similarity
        std::map<std::string, int> counter1, counter2;
        for (const auto& line : preprocessed_lines1) counter1[line]++;
        for (const auto& line : preprocessed_lines2) counter2[line]++;

        size_t common = 0;
        for (const auto& [line, count] : counter1) {
            auto it = counter2.find(line);
            if (it != counter2.end()) {
                common += std::min(count, it->second);
            }
        }

        const size_t max_lines = std::max(lines1.size(), lines2.size());
        out.ratio = max_lines ? static_cast<double>(common) / max_lines : 0.0;
        out.duplicates = common;
        out.total1 = lines1.size();
        out.total2 = lines2.size();
        out.filename = file1.filename().string();

        // Generate diff
        if (show_diff_) {
            size_t i = 0, j = 0;
            while (i < preprocessed_lines1.size() && j < preprocessed_lines2.size()) {
                if (preprocessed_lines1[i] == preprocessed_lines2[j]) {
                    ++i;
                    ++j;
                } else {
                    out.diff.push_back(colorize("- " + lines1[i], RED));
                    ++i;
                    out.diff.push_back(colorize("+ " + lines2[j], GREEN));
                    ++j;
                }
            }
            while (i < preprocessed_lines1.size()) {
                out.diff.push_back(colorize("- " + lines1[i], RED));
                ++i;
            }
            while (j < preprocessed_lines2.size()) {
                out.diff.push_back(colorize("+ " + lines2[j], GREEN));
                ++j;
            }
        }

        return out;
    }

    std::pair<FileResult, std::vector<FileResult>> compare_directories(
        const fs::path& dir1, const fs::path& dir2) const
    {
        std::map<std::string, fs::path> files1, files2;

        // Build file maps
        auto build_index = [this](const fs::path& dir) {
            std::map<std::string, fs::path> index;
            try {
                for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                    if (entry.is_regular_file() && is_text_file(entry.path()) && !is_excluded(entry.path())) {
                        try {
                            index[fs::relative(entry.path(), dir).string()] = entry.path();
                        } catch (...) { }
                    }
                }
            } catch (...) { }
            return index;
            };

        files1 = build_index(dir1);
        files2 = build_index(dir2);

        // Compare common files
        FileResult total;
        std::vector<FileResult> details;

        for (const auto& [rel_path, path1] : files1) {
            auto it = files2.find(rel_path);
            if (it != files2.end()) {
                auto res = compare_files(path1, it->second);
                total.duplicates += res.duplicates;
                total.total1 += res.total1;
                total.total2 += res.total2;
                details.push_back(std::move(res));
            }
        }

        const size_t max_total = std::max(total.total1, total.total2);
        total.ratio = max_total ? static_cast<double>(total.duplicates) / max_total : 0.0;
        total.filename = "TOTAL";

        return { total, details };
    }

    void print_results(const FileResult& total,
        const std::vector<FileResult>& details) const
    {
        // Output differences
        if (show_diff_ && !details.empty()) {
            std::cout << colorize("\n=== DIFFERENCES ===", CYAN) << "\n";
            for (const auto& res : details) {
                if (!res.diff.empty()) {
                    std::cout << colorize("\n--- " + res.filename + " ---", CYAN) << "\n";
                    for (const auto& line : res.diff) {
                        std::cout << line << "\n";
                    }
                }
            }
        }

        // Output file list
        std::cout << colorize("\n=== FILES ===", CYAN) << "\n";
        char buf[64];
        for (const auto& res : details) {
            const std::string& color = res.ratio > 0.8 ? GREEN :
                res.ratio > 0.4 ? YELLOW : RED;
            snprintf(buf, sizeof(buf), "%-50s %d%%", res.filename.c_str(),
                static_cast<int>(res.ratio * 100));
            std::cout << colorize(buf, color) << "\n";
        }

        // Output summary
        std::cout << colorize("\n=== ANALYSIS SUMMARY ===", CYAN) << "\n"
            << colorize("Matched files: " + std::to_string(details.size()), YELLOW) << "\n"
            << colorize("Similarity: " + std::to_string(static_cast<int>(total.ratio * 100)) + "%", YELLOW) << "\n"
            << colorize("Duplicate lines: " + std::to_string(total.duplicates)
                + " (S: " + std::to_string(total.total1) + ", T: " + std::to_string(total.total2) + ")", MAGENTA) << "\n";
    }
};

void print_usage(const std::string& exe)
{
    std::cout << "Usage: " << exe << " <source> <target> [--exclude <path>] [--diff] [--no-color]\n"
        << "  <source> and <target>\n"
        << "               can be both files or directories\n"
        << "  --exclude    Exclude the given directory or file from analysis\n"
        << "  --diff       Show detailed differences\n"
        << "  --no-color   Disable colored output\n";
}

int main(int argc, char* argv[])
{
    // If no arguments or help requested, show usage
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    bool use_color = true;
    bool show_diff = false;
    fs::path source, target;
    std::vector<fs::path> exclude_paths{};

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--exclude") {
            if (i + 1 < argc) {
                exclude_paths.push_back(argv[++i]);
            } else {
                std::cerr << "Error: --exclude requires a path argument\n";
                return 1;
            }
        } else if (arg == "--diff") {
            show_diff = true;
        } else if (arg == "--no-color") {
            use_color = false;
        } else if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return 0;
        } else if (source.empty()) {
            source = arg;
        } else {
            target = arg;
        }
    }

    if (source.empty() || target.empty()) {
        print_usage(argv[0]);
        return 1;
    }

    try {
        FileAnalyzer analyzer(use_color, show_diff, exclude_paths);

        if (fs::is_regular_file(source) && fs::is_regular_file(target)) {
            auto res = analyzer.compare_files(source, target);
            analyzer.print_results(res, { res });
        } else if (fs::is_directory(source) && fs::is_directory(target)) {
            auto [total, details] = analyzer.compare_directories(source, target);
            analyzer.print_results(total, details);
        } else {
            std::cerr << "Error: Requires two files or two directories\n";
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
