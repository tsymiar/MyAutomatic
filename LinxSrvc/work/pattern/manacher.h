#pragma once
#include <vector>
#include <string>
#include <fstream>

class ManacherFileProcessor {
public:
    ManacherFileProcessor(size_t chunkSize);

    // 预处理数据，将每个字符之间插入特殊分隔符
    void preprocessData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);

    // 处理每个块
    void processChunk(const std::vector<uint8_t>& chunk, size_t fileOffset);

    // 查找最长回文
    std::pair<size_t, std::vector<uint8_t>> findLongestPalindrome(const std::string& filePath);

private:
    size_t maxLen_;
    size_t maxPos_;
    size_t chunkSize_;
    std::vector<uint8_t> maxData_;
    std::vector<uint8_t> lastOverlap_;
};

/*
// Usage
#include "manacher_file.h"
#include <iostream>
int main() {
    try {
        ManacherFileProcessor processor(16 * 1024 * 1024); // 16MB
        auto result = processor.findLongestPalindrome("large_text.txt");
        std::cout << "Longest palindrome at position: " << result.first
                  << "\nContent: " << result.second << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
*/
