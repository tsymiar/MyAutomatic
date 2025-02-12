#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <map>
#include <algorithm>
#include <iomanip>
#include <sstream>

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
    std::vector<std::string> diff;
    std::string filename;
};

class CodeAnalyzer {
    bool use_color_;
    bool show_diff_;

    static bool is_text_file(const fs::path& path) {
        static const std::vector<std::string> TEXT_EXTS = {
            ".cpp", ".h", ".hpp", ".c", ".cc", ".py", 
            ".java", ".txt", ".md", ".json", ".xml"
        };
        const std::string ext = path.extension().string();
        return std::any_of(TEXT_EXTS.begin(), TEXT_EXTS.end(),
            [&ext](const auto& e) { 
                return std::equal(ext.rbegin(), ext.rend(),
                                 e.rbegin(), e.rend(),
                                 [](char a, char b) {
                                     return tolower(a) == tolower(b);
                                 });
            });
    }

    static std::vector<std::string> read_lines(const fs::path& path) {
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

    std::string colorize(const std::string& text, const char* color) const {
        return use_color_ ? (color + text + RESET) : text;
    }

public:
    CodeAnalyzer(bool use_color, bool show_diff)
        : use_color_(use_color), show_diff_(show_diff) {}

    FileResult compare_files(const fs::path& file1, const fs::path& file2) const {
        FileResult out;
        const auto lines1 = read_lines(file1);
        const auto lines2 = read_lines(file2);

        // Calculate similarity
        std::map<std::string, int> counter1, counter2;
        for (const auto& line : lines1) counter1[line]++;
        for (const auto& line : lines2) counter2[line]++;
        
        size_t common = 0;
        for (const auto& [line, count] : counter1) {
            if (counter2.count(line)) {
                common += std::min(count, counter2[line]);
            }
        }
        
        const size_t max_lines = std::max(lines1.size(), lines2.size());
        out.ratio = max_lines ? static_cast<double>(common)/max_lines : 0.0;
        out.duplicates = common;
        out.total1 = lines1.size();
        out.total2 = lines2.size();
        out.filename = file1.filename().string();

        // Generate diff
        if (show_diff_) {
            size_t i = 0, j = 0;
            while (i < lines1.size() && j < lines2.size()) {
                if (lines1[i] == lines2[j]) {
                    ++i;
                    ++j;
                } else {
                    out.diff.push_back(colorize("- " + lines1[i], RED));
                    ++i;
                    out.diff.push_back(colorize("+ " + lines2[j], GREEN));
                    ++j;
                }
            }
            while (i < lines1.size()) {
                out.diff.push_back(colorize("- " + lines1[i], RED));
                ++i;
            }
            while (j < lines2.size()) {
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
        auto build_index = [](const fs::path& dir) {
            std::map<std::string, fs::path> index;
            try {
                for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                    if (entry.is_regular_file() && is_text_file(entry.path())) {
                        try {
                            index[fs::relative(entry.path(), dir).string()] = entry.path();
                        } catch (...) {} // Ignore path errors
                    }
                }
            } catch (...) {} // Ignore directory errors
            return index;
        };
        
        files1 = build_index(dir1);
        files2 = build_index(dir2);

        // Compare common files
        FileResult total;
        std::vector<FileResult> details;
        
        for (const auto& [rel_path, path1] : files1) {
            if (files2.count(rel_path)) {
                auto res = compare_files(path1, files2.at(rel_path));
                total.duplicates += res.duplicates;
                total.total1 += res.total1;
                total.total2 += res.total2;
                details.push_back(std::move(res));
            }
        }

        const size_t max_total = std::max(total.total1, total.total2);
        total.ratio = max_total ? static_cast<double>(total.duplicates)/max_total : 0.0;
        total.filename = "TOTAL";
        
        return {total, details};
    }

    void print_results(const FileResult& total, 
                      const std::vector<FileResult>& details) const {
        // Print diffs
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

        // Print summary
        std::cout << colorize("\n=== ANALYSIS SUMMARY ===", CYAN) << "\n"
                  << colorize("Matched files: " + std::to_string(details.size()), YELLOW) << "\n"
                  << colorize("Similarity: " + std::to_string(static_cast<int>(total.ratio * 100)) + "%", YELLOW) << "\n"
                  << colorize("Duplicate lines: " + std::to_string(total.duplicates), MAGENTA) << "\n";

        // Print file list
        std::cout << colorize("\n=== FILES ===", CYAN) << "\n";
        for (const auto& res : details) {
            const std::string color = res.ratio > 0.7 ? GREEN : 
                                    res.ratio > 0.3 ? YELLOW : RED;
            std::ostringstream oss;
            oss << std::setw(50) << std::left << res.filename 
            << " " << static_cast<int>(res.ratio * 100) << "%";
            std::cout << colorize(oss.str(), color.c_str()) << "\n";
        }
    }
};

int main(int argc, char* argv[]) {
    bool use_color = true;
    bool show_diff = false;
    fs::path source, target;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-color") use_color = false;
        else if (arg == "--diff") show_diff = true;
        else if (source.empty()) source = arg;
        else target = arg;
    }

    try {
        CodeAnalyzer analyzer(use_color, show_diff);
        
        if (fs::is_regular_file(source) && fs::is_regular_file(target)) {
            auto res = analyzer.compare_files(source, target);
            analyzer.print_results(res, {res});
        }
        else if (fs::is_directory(source) && fs::is_directory(target)) {
            auto [total, details] = analyzer.compare_directories(source, target);
            analyzer.print_results(total, details);
        }
        else {
            std::cerr << "Error: Requires two files or two directories\n";
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}

