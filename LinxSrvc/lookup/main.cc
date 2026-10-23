#include "manacher.h"
#include "kmp.h"
#include <iostream>
#include <ostream>

#include <vector>
#include <string>

void printUsage(const char* prog, std::ostream& os)
{
    os << "Usage:\n"
        << "  " << prog << " <filePath>            Scan a file and print its longest palindrome\n"
        << "  " << prog << " <filePath> <pattern>  Search a file for a hex byte pattern (KMP)\n"
        << "  " << prog << " -h | --help           Print this help\n"
        << "\nArguments:\n"
        << "  filePath  Path of the file to scan, opened in binary mode\n"
        << "  pattern   Hex byte sequence, e.g. deadbeef / 0xdeadbeef /0xDEADBEEF\n"
        << "            Must contain an even number of hex digits\n"
        << "\nExamples:\n"
        << "  " << prog << " data.bin\n"
        << "  " << prog << " data.bin deadbeef\n"
        << "  " << prog << " data.bin 0xdeadbeef\n"
        << "\nOutput:\n"
        << "  Palindrome mode  Longest palindrome at position: <start offset>\n"
        << "                   Content: <palindrome bytes in hex, 16 bytes per line>\n"
        << "  Search mode      At <index>, offset: <match offset>, size: <pattern length>\n"
        << "                   At most 1000 matches are reported\n"
        << std::endl;
}

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

void processPalindromeFile(const std::string& filePath)
{
    try {
        ManacherFileProcessor processor(16 * 1024 * 1024); // 16MB
        auto target = processor.findLongestPalindrome(filePath);
        std::cout << "Longest palindrome at position: " << target.first
            << "\nContent: " << parseVector(target.second) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int processSearchFile(const std::string& filePath, const char* patternText)
{
    auto vecBin = hexStringToBinary(patternText);
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
    std::cout << (found > 0 ? "Has" : "Not") << " found pattern: [" << parseVector(vecBin) << "]" << std::endl;
    for (size_t i = 0; i < found; ++i) {
        printf("At %zu, size: %zu, offset: 0x%08zx(%zu)\n", i + 1,
            frames[i].length,
            frames[i].offset,
            frames[i].offset);
    }
    kmp_free(kmp);
    return 0;
}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        printUsage(argv[0], std::cerr);
        return 1;
    }
    std::string filePath = argv[1];
    if (filePath == "-h" || filePath == "--help" || filePath == "help") {
        printUsage(argv[0], std::cout);
        return 0;
    }
    std::cout << "Processing file: " << filePath << std::endl;

    if (argc < 3) {
        processPalindromeFile(filePath);
        return 0;
    }
    return processSearchFile(filePath, argv[2]);
}
