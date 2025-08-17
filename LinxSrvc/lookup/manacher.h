#pragma once
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

class ManacherFileProcessor {
public:
    ManacherFileProcessor(size_t chunkSize);

    // Preprocess data by inserting a special separator between each character
    void preprocessData(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst);

    // Process each chunk
    void processChunk(const std::vector<uint8_t>& chunk, size_t fileOffset);

    // Find the specified uint64_t data, return nullptr if not found
    const uint64_t* find_uint64(const uint64_t* val, size_t len, uint64_t dst, bool mem = true);

    // Find the longest palindrome
    std::pair<size_t, std::vector<uint8_t>> findLongestPalindrome(const std::string& filePath);

private:
    size_t maxLen_;
    size_t maxPos_;
    size_t chunkSize_;
    std::vector<uint8_t> maxData_;
    std::vector<uint8_t> lastOverlap_;
};
