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

    // 查找指定uint64_t数据，未找到返回nullptr
    const uint64_t* find_uint64(const uint64_t* val, size_t len, uint64_t dst, bool mem = true);
    // 查找最长回文
    std::pair<size_t, std::vector<uint8_t>> findLongestPalindrome(const std::string& filePath);

private:
    size_t maxLen_;
    size_t maxPos_;
    size_t chunkSize_;
    std::vector<uint8_t> maxData_;
    std::vector<uint8_t> lastOverlap_;
};
