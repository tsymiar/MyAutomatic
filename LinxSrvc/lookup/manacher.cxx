#include "manacher.h"
#include <algorithm>
#include <stdexcept>

ManacherFileProcessor::ManacherFileProcessor(size_t chunkSize)
    : maxLen_(0), maxPos_(0), chunkSize_(chunkSize)
{ }

void ManacherFileProcessor::preprocessData(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst)
{
    dst.clear();
    dst.push_back(0xFF); // Start with a sentinel value
    for (auto c : src) {
        dst.push_back(c);
        dst.push_back(0xFF);
    }
}

void ManacherFileProcessor::processChunk(const std::vector<uint8_t>& chunk, size_t fileOffset)
{
    std::vector<uint8_t> processed;
    preprocessData(chunk, processed);

    std::vector<int> p(processed.size(), 0);
    int center = 0, right = 0;

    for (int i = 0; i < (int)processed.size(); ++i) {
        int mirror = 2 * center - i;
        if (i < right) {
            p[i] = std::min(right - i, p[mirror]);
        }

        int a = i + (1 + p[i]);
        int b = i - (1 + p[i]);
        while (a < (int)processed.size() && b >= 0 && processed[a] == processed[b]) {
            p[i]++;
            a++;
            b--;
        }

        if (i + p[i] > right) {
            center = i;
            right = i + p[i];
        }

        if ((size_t)p[i] > maxLen_) {
            maxLen_ = p[i];
            maxPos_ = fileOffset + (i - p[i]) / 2;
            maxData_.assign(chunk.begin() + (i - p[i]) / 2, chunk.begin() + (i - p[i]) / 2 + p[i]);
        }
    }
}

std::pair<size_t, std::vector<uint8_t>> ManacherFileProcessor::findLongestPalindrome(
    const std::string& filePath)
{
    std::ifstream file(filePath, std::ios::binary);
    if (!file) throw std::runtime_error("File open failed!");

    std::vector<uint8_t> buffer(chunkSize_);
    size_t overlapSize = 100;
    size_t globalOffset = 0;

    while (file) {
        file.read(reinterpret_cast<char*>(buffer.data()), chunkSize_);
        size_t bytesRead = file.gcount();
        if (bytesRead == 0) break;

        std::vector<uint8_t> chunk(buffer.begin(), buffer.begin() + bytesRead);
        if (!lastOverlap_.empty()) {
            chunk.insert(chunk.begin(), lastOverlap_.begin(), lastOverlap_.end());
        }

        processChunk(chunk, globalOffset - (globalOffset > 0 ? overlapSize : 0));

        // Handle overlap for the next chunk
        if (chunk.size() > overlapSize)
            lastOverlap_.assign(chunk.end() - overlapSize, chunk.end());
        else
            lastOverlap_ = chunk;

        globalOffset += bytesRead;
    }

    return { maxPos_, maxData_ };
}

// Simulated database lookup function (should be replaced with a real database interface in production)
static const uint64_t* db_lookup(const uint64_t* val, size_t len, uint64_t dst)
{
    return nullptr;
}

const uint64_t* ManacherFileProcessor::find_uint64(const uint64_t* val, size_t len, uint64_t dst, bool mem)
{
    // 1. First, search in memory or database
    if (mem) {
        for (size_t i = 0; i < len; ++i) {
            if (val[i] == dst) return &val[i];
        }
    } else {
        return db_lookup(val, len, dst);
    }
    // 2. Binary search (assuming val is sorted)
    size_t first = 0, last = len;
    while (first < last) {
        size_t mid = first + (last - first) / 2;
        if (val[mid] == dst) return &val[mid];
        else if (val[mid] < dst) first = mid + 1;
        else last = mid;
    }
    return nullptr;
}
// If a pattern is provided, perform KMP search
