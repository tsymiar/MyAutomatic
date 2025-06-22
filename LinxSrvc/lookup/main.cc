#include "manacher.h"
#include "kmp.h"
#include <iostream>

#include <vector>
#include <string>

template <typename T>
std::string parseVector(const std::vector<T>& vec)
{
    int count = 0;
    std::string dst;
    for (const auto& elem : vec) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X ", static_cast<unsigned int>(elem));
        dst += buf;
        if (count % 16 == 0xf) {
            dst += "\n";
        }
        count++;
    }
    while (!dst.empty() && (dst.back() == ' ' || dst.back() == '\n')) {
        dst.pop_back();
    }
    return dst;
}

std::vector<uint8_t> hexStringToBinary(const std::string& hex)
{
    std::vector<uint8_t> vec;
    std::string cleanHex = hex;

    // Remove "0x" or "0X" prefix if present
    if (cleanHex.size() >= 2 && (cleanHex[0] == '0') && (cleanHex[1] == 'x' || cleanHex[1] == 'X')) {
        cleanHex = cleanHex.substr(2);
    }

    size_t len = cleanHex.length();
    if (len % 2 != 0) return vec;

    for (size_t i = 0; i < len; i += 2) {
        uint8_t byte = 0;
        try {
            byte = static_cast<uint8_t>(std::stoul(cleanHex.substr(i, 2), nullptr, 16));
        } catch (...) {
            return std::vector<uint8_t>();
        }
        vec.push_back(byte);
    }
    return vec;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filePath> [pattern]" << std::endl;
        return 1;
    }
    std::string filePath = argv[1];
    std::cout << "Processing file: " << filePath << std::endl;

    if (argc < 3) {
        try {
            ManacherFileProcessor processor(16 * 1024 * 1024); // 16MB
            auto target = processor.findLongestPalindrome(filePath);
            std::cout << "Longest palindrome at position: " << target.first
                << "\nContent: " << parseVector(target.second) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    } else {
        auto vecBin = hexStringToBinary(argv[2]);
        unsigned char* pattern = nullptr;
        if (!vecBin.empty()) {
            pattern = reinterpret_cast<unsigned char*>(vecBin.data());
        }
        KMPDetail* kmp = kmp_create(pattern, vecBin.size());
        if (!kmp) {
            std::cerr << "Failed to create KMP pattern." << std::endl;
            return 1;
        }
        MatchedFrame frames[1000];
        size_t found = kmp_get_frame(kmp, filePath.c_str(), frames, 1000);
        std::cout << "Found pattern: [" << parseVector(vecBin) << "]" << std::endl;
        for (size_t i = 0; i < found; ++i) {
            printf("At %zu, offset: %zu, size: %zu\n", i + 1,
                frames[i].offset,
                frames[i].length);
        }
        kmp_free(kmp);
    }
    return 0;
}
