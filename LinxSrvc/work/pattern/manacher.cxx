#include "manacher.h"
#include <algorithm>
#include <stdexcept>
#include <cstdint>

ManacherFileProcessor::ManacherFileProcessor(size_t chunkSize)
    : maxLen_(0), maxPos_(0), chunkSize_(chunkSize)
{ }

void ManacherFileProcessor::preprocessData(const std::vector<uint8_t>& input, std::vector<uint8_t>& output)
{
    output.clear();
    output.push_back(0xFF); // Start with a sentinel value
    for (auto c : input) {
        output.push_back(c);
        output.push_back(0xFF);
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
    if (!file) throw std::runtime_error("File open failed");

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
