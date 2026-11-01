#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

    // Cap for the Myers backtrack table (int32 cells); larger diffs fall back to line sync.
    static constexpr size_t kMaxMyersCells = 4u << 20;

    enum class DiffOp { Equal, Remove, Add };
    struct DiffEntry { DiffOp op; size_t index; };

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

    // Strip comments and whitespace in a single pass.
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
                    i++; // skip the closing '/'
                }
                continue;
            }
            if (c == '/' && i + 1 < line.size()) {
                if (line[i + 1] == '*') {
                    in_block_comment = true;
                    i++; // skip '*'
                    continue;
                }
                if (line[i + 1] == '/') break; // line comment: drop the rest
            }
            if (!std::isspace(static_cast<unsigned char>(c))) {
                content.push_back(c);
            }
        }
        return content;
    }

    static std::vector<std::string> strip_lines(const std::vector<std::string>& lines)
    {
        std::vector<std::string> out;
        out.reserve(lines.size());
        for (const auto& line : lines) out.push_back(get_effective_content(line));
        return out;
    }

    // Fallback: walk both sides in lockstep, emitting - and + for every mismatch.
    static void append_sync(std::vector<DiffEntry>& script,
        const std::vector<std::string>& a, size_t a0, size_t n,
        const std::vector<std::string>& b, size_t b0, size_t m)
    {
        size_t i = 0, j = 0;
        while (i < n && j < m) {
            if (a[a0 + i] == b[b0 + j]) {
                ++i;
                ++j;
            } else {
                script.push_back({ DiffOp::Remove, a0 + i });
                ++i;
                script.push_back({ DiffOp::Add, b0 + j });
                ++j;
            }
        }
        while (i < n) { script.push_back({ DiffOp::Remove, a0 + i }); ++i; }
        while (j < m) { script.push_back({ DiffOp::Add, b0 + j }); ++j; }
    }

    // Myers greedy: O((N+M)*D). Returns false if the backtrack table would blow the cap.
    static bool myers_script(const std::vector<std::string>& a, size_t a0, size_t a1,
        const std::vector<std::string>& b, size_t b0, size_t b1,
        std::vector<DiffEntry>& out)
    {
        const int32_t n = static_cast<int32_t>(a1 - a0);
        const int32_t m = static_cast<int32_t>(b1 - b0);
        const int32_t max_d = n + m;
        const int32_t offset = max_d;
        std::vector<int32_t> v(static_cast<size_t>(2 * max_d + 1), 0);
        std::vector<std::vector<int32_t>> trace;
        size_t cells = 0;

        for (int32_t d = 0; d <= max_d; ++d) {
            const size_t width = static_cast<size_t>(2 * d + 1);
            if (cells + width > kMaxMyersCells) return false;
            trace.emplace_back();
            trace.back().reserve(width);
            for (int32_t k = -d; k <= d; ++k) trace.back().push_back(v[offset + k]);
            cells += width;

            for (int32_t k = -d; k <= d; k += 2) {
                int32_t x = (k == -d || (k != d && v[offset + k - 1] < v[offset + k + 1]))
                    ? v[offset + k + 1]
                    : v[offset + k - 1] + 1;
                int32_t y = x - k;
                while (x < n && y < m && a[a0 + static_cast<size_t>(x)] == b[b0 + static_cast<size_t>(y)]) {
                    ++x;
                    ++y;
                }
                v[offset + k] = x;
                if (x >= n && y >= m) {
                    myers_backtrack(trace, x, y, a0, b0, d, out);
                    return true;
                }
            }
        }
        return false;
    }

    // Walk back from the end: trace[d] is V before step d, which gives the previous position.
    static void myers_backtrack(const std::vector<std::vector<int32_t>>& trace,
        int32_t x, int32_t y, size_t a0, size_t b0, int32_t start_d,
        std::vector<DiffEntry>& out)
    {
        for (int32_t d = start_d; d >= 1; --d) {
            const auto& v = trace[static_cast<size_t>(d)];
            const int32_t k = x - y;
            const int32_t prev_k =
                (k == -d || (k != d && v[k - 1 + d] < v[k + 1 + d])) ? k + 1 : k - 1;
            const int32_t prev_x = v[prev_k + d];
            const int32_t prev_y = prev_x - prev_k;
            while (x > prev_x && y > prev_y) {
                --x;
                --y;
                out.push_back({ DiffOp::Equal, a0 + static_cast<size_t>(x) });
            }
            if (x == prev_x) {
                out.push_back({ DiffOp::Add, b0 + static_cast<size_t>(prev_y) });
            } else {
                out.push_back({ DiffOp::Remove, a0 + static_cast<size_t>(prev_x) });
            }
            x = prev_x;
            y = prev_y;
        }
        // Whatever is left lies on the starting diagonal: all equal.
        while (x > 0 && y > 0) {
            --x;
            --y;
            out.push_back({ DiffOp::Equal, a0 + static_cast<size_t>(x) });
        }
    }

    // Trim the common prefix/suffix, then diff the middle with Myers.
    static std::vector<DiffEntry> build_edit_script(
        const std::vector<std::string>& a, const std::vector<std::string>& b)
    {
        std::vector<DiffEntry> script;
        script.reserve(a.size() + b.size());

        size_t prefix = 0;
        while (prefix < a.size() && prefix < b.size() && a[prefix] == b[prefix]) ++prefix;
        // Clamp the remainders to avoid unsigned underflow (size_t) when one
        // sequence is a prefix/suffix of the other.
        const size_t a_mid = a.size() > prefix ? a.size() - prefix : 0;
        const size_t b_mid = b.size() > prefix ? b.size() - prefix : 0;
        size_t suffix = 0;
        while (suffix < a_mid && suffix < b_mid &&
               a[a.size() - 1 - suffix] == b[b.size() - 1 - suffix]) ++suffix;

        const size_t n = a_mid > suffix ? a_mid - suffix : 0;
        const size_t m = b_mid > suffix ? b_mid - suffix : 0;

        for (size_t k = 0; k < prefix; ++k) script.push_back({ DiffOp::Equal, k });

        if (n > 0 && m > 0) {
            std::vector<DiffEntry> mid;
            if (myers_script(a, prefix, prefix + n, b, prefix, prefix + m, mid)) {
                std::reverse(mid.begin(), mid.end());
                script.insert(script.end(), mid.begin(), mid.end());
            } else {
                append_sync(script, a, prefix, n, b, prefix, m);
            }
        } else {
            append_sync(script, a, prefix, n, b, prefix, m);
        }

        for (size_t k = 0; k < suffix; ++k) script.push_back({ DiffOp::Equal, a.size() - suffix + k });
        return script;
    }

    std::string colorize(const std::string& text, const char* color) const
    {
        return use_color_ ? (color + text + RESET) : text;
    }

    // Compare absolute paths so ./a, a and relative --exclude values all line up.
    static fs::path normalize_path(const fs::path& path)
    {
        if (path.empty()) return path;
        std::error_code ec;
        fs::path canonical = fs::weakly_canonical(path, ec);
        return ec ? fs::absolute(path) : canonical;
    }

    static bool is_sub_path(const fs::path& path, const fs::path& base)
    {
        auto pit = path.begin();
        for (auto bit = base.begin(); bit != base.end(); ++bit, ++pit) {
            if (pit == path.end() || *pit != *bit) return false;
        }
        return true;
    }

    bool is_excluded(const fs::path& path) const
    {
        if (exclude_paths_.empty()) return false;
        const fs::path p = normalize_path(path);
        for (const auto& exclude_path : exclude_paths_) {
            if (p == exclude_path || is_sub_path(p, exclude_path)) return true;
        }
        return false;
    }

public:
    FileAnalyzer(bool use_color, bool show_diff, const std::vector<fs::path>& exclude_paths = {})
        : use_color_(use_color), show_diff_(show_diff)
    {
        exclude_paths_.reserve(exclude_paths.size());
        for (const auto& path : exclude_paths) exclude_paths_.push_back(normalize_path(path));
    }

    FileResult compare_files(const fs::path& file1, const fs::path& file2) const
    {
        FileResult out;
        const auto lines1 = read_lines(file1);
        const auto lines2 = read_lines(file2);

        // Effective content: comments and whitespace removed.
        const auto eff1 = strip_lines(lines1);
        const auto eff2 = strip_lines(lines2);

        // Blank and comment-only lines reduce to empty; counting them inflates similarity.
        std::unordered_map<std::string, size_t> counter1, counter2;
        counter1.reserve(eff1.size());
        counter2.reserve(eff2.size());
        for (const auto& line : eff1) if (!line.empty()) { counter1[line]++; ++out.total1; }
        for (const auto& line : eff2) if (!line.empty()) { counter2[line]++; ++out.total2; }

        // Iterate the smaller map to keep lookups down.
        const std::unordered_map<std::string, size_t>* smaller = &counter1;
        const std::unordered_map<std::string, size_t>* larger = &counter2;
        if (smaller->size() > larger->size()) std::swap(smaller, larger);
        for (const auto& [line, count] : *smaller) {
            auto it = larger->find(line);
            if (it != larger->end()) {
                out.duplicates += std::min(count, it->second);
            }
        }

        const size_t max_lines = std::max(out.total1, out.total2);
        out.ratio = max_lines ? static_cast<double>(out.duplicates) / max_lines : 0.0;
        out.filename = file1.filename().string();

        // Emit the - / + lines.
        if (show_diff_) {
            for (const auto& entry : build_edit_script(eff1, eff2)) {
                if (entry.op == DiffOp::Remove) {
                    out.diff.push_back(colorize("- " + lines1[entry.index], RED));
                } else if (entry.op == DiffOp::Add) {
                    out.diff.push_back(colorize("+ " + lines2[entry.index], GREEN));
                }
            }
        }

        return out;
    }

    std::pair<FileResult, std::vector<FileResult>> compare_directories(
        const fs::path& dir1, const fs::path& dir2) const
    {
        std::map<std::string, fs::path> files1, files2;

        // Relative path -> full path.
        auto build_index = [this](const fs::path& dir) {
            std::map<std::string, fs::path> index;
            try {
                fs::recursive_directory_iterator it(
                    dir, fs::directory_options::skip_permission_denied), end;
                for (; it != end; ++it) {
                    const fs::path& path = it->path();
                    // Prune excluded directories instead of walking the subtree.
                    if (it->is_directory()) {
                        if (is_excluded(path)) it.disable_recursion_pending();
                        continue;
                    }
                    if (!it->is_regular_file() || is_excluded(path)) continue;
                    if (!is_text_file(path)) continue;
                    std::error_code ec;
                    std::string rel = fs::relative(path, dir, ec).string();
                    if (ec) continue;
                    index[rel] = path;
                }
            } catch (const fs::filesystem_error&) { }
            return index;
            };

        files1 = build_index(dir1);
        files2 = build_index(dir2);

        // Compare common files
        FileResult total;
        std::vector<FileResult> details;
        details.reserve(std::min(files1.size(), files2.size()));

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
            const char* color = res.ratio > 0.8 ? GREEN :
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
        for (int i = 1; i < argc; ++i) {
            if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") return 0;
        }
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
        } else if (target.empty()) {
            target = arg;
        } else {
            std::cerr << "Error: unexpected argument '" << arg << "'\n";
            print_usage(argv[0]);
            return 1;
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
