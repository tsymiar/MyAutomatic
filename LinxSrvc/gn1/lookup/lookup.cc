#include "manacher.h"
#include "kmp.h"
#include <iostream>

#include <vector>
#include <string>

template <typename T>
std::string vectorToString(const std::vector<T>& vec)
{
    std::string dst;
    for (const auto& elem : vec) {
        dst += "[" + std::to_string(elem) + "] ";
    }
    return dst;
}

std::vector<uint8_t> hexStringToBinary(const std::string& hex)
{
    std::vector<uint8_t> vec;
    size_t len = hex.length();
    if (len % 2 != 0) return vec;

    for (size_t i = 0; i < len; i += 2) {
        uint8_t byte = 0;
        try {
            byte = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
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
                << "\nContent: " << vectorToString(target.second) << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    } else {
        auto vecBin = hexStringToBinary(argv[2]);
        char* pattern = nullptr;
        if (!vecBin.empty()) {
            pattern = reinterpret_cast<char*>(vecBin.data());
        }
        KMPSearch* kmp = kmp_create(pattern);
        size_t offset[1000];
        size_t found = kmp_search(kmp, filePath.c_str(), 4096, offset, 1000);
        for (size_t i = 0; i < found; ++i) {
            printf("Found at: %zu\n", offset[i]);
        }
        kmp_free(kmp);
    }
    return 0;
}
