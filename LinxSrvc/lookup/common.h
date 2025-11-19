#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sys/time.h>

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 256
#endif

#define DELETE(x) do { \
        if (x) {       \
            delete x;  \
            x = NULL;  \
        }              \
    } while (0)

#define MIN_JUDGE_FRAME_SIZE (0x40)
#define MIN_FRAME_SIZE 0x10
#define PROJECT_FILE_OFFSET (80)
#define CONST_FRAME_HEAD (0x1234567890abcdefULL)
#define SEEK_FRAME_HEAD (0x1122334455667788ULL)

struct ProjectFrame {
    uint64_t syncHead;
    uint64_t utctime;
    uint64_t size;
    ProjectFrame()
    {
        syncHead = SEEK_FRAME_HEAD;
        utctime = 0;
        size = 0;
    }
};

struct UserFileFrameHeader {
    uint64_t header;
    uint64_t timestamp;
    uint32_t id;
    uint32_t len;
    uint64_t tail;
    UserFileFrameHeader()
    {
        header = CONST_FRAME_HEAD;
        id = 0;
        len = 0;
        timestamp = 0;
    }
};

struct SeekTimeValue {
    uint64_t timestamp;
    uint64_t offset;
    uint64_t size;
    SeekTimeValue()
    {
        timestamp = offset = size = 0;
    }
};

#pragma pack(push)
#pragma pack(4)
typedef struct tagSeekTimeContent {
    char fileName[MAX_PATH_LEN];
    uint32_t fileid = 1;
    uint64_t totalSize;
    int32_t duration;
    SeekTimeValue value;
    bool found;
    uint64_t param;
    uint32_t reserve;
} SeekTimeContent;
#pragma pack(pop)

struct SelectValue {
    uint64_t first;
    uint64_t last;
    const uint64_t average()
    {
        return (this->first + this->last) / 2;
    }
    bool operator==(const SelectValue& v) const
    {
        return ((first == v.first) && (last == v.last));
    }
    SelectValue& operator = (const SelectValue& v)
    {
        this->first = v.first > 0 ? v.first : 0;
        this->last = v.last > 0 ? v.last : 0;
        return *this;
    }
    void fix()
    {
        if (int64_t(this->first) < 0) {
            this->first = 0;
        } else
            if (this->first > this->last) {
                uint64_t value = this->first;
                this->first = this->last;
                this->last = value;
            } else if (this->first != 0 && this->last == 0) {
                this->last = this->first;
            }
    }
};

typedef SelectValue SelectTime;
typedef SelectValue SelectOffset;
typedef SelectValue FileDataTime;
typedef SelectValue FileDataOffset;

struct FileTimeDetails {
    FileDataTime time;
    FileDataOffset offset;
};

static uint64_t getUsecTime()
{
    uint64_t usec = 0;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    usec = tv.tv_sec * 1000000ULL + tv.tv_usec;
    return usec;
}

static void* memset16(void* ptr, uint16_t value, size_t num_pairs)
{
#ifdef BIG_ENDIAN
    uint8_t* p = (uint8_t*)ptr;
#else
    uint16_t* p = (uint16_t*)ptr;
#endif
    for (size_t i = 0; i < num_pairs / 2; i++) {
#ifdef BIG_ENDIAN
        p[2 * i] = (value >> 8) & 0xFF;
        p[2 * i + 1] = value & 0xFF;
#else
        p[i] = value;
#endif
    }
    return ptr;
}

static std::string getFileAsString(const std::string& filename)
{
    std::string content = "";
    std::ifstream file(filename);
    if (file.is_open()) {
        content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
    file.close();
    return content;
}

static std::vector<std::string> splitLines(const std::string& str)
{
    std::vector<std::string> res{};
    if (str.empty()) {
        return res;
    }
    size_t start = 0;
    while (true) {
        size_t pos = str.find('\n', start);
        if (pos != std::string::npos) {
            if (pos > 0 && str[pos - 1] == '\r') {
                std::string val = str.substr(start, pos - 1 - start);
                if (!val.empty())
                    res.push_back(val);
            } else {
                std::string val = str.substr(start, pos - start);
                if (!val.empty())
                    res.push_back(val);
            }
            start = pos + 1;
        } else {
            std::string val = str.substr(start);
            if (!val.empty())
                res.push_back(val);
            break;
        }
    }
    return res;
}

static std::string getVariable(const std::string& src, const std::string& key)
{
    std::string val = "";
    size_t pos = src.find(key);
    if (pos != std::string::npos) {
        val = src.substr(pos, src.size());
        pos = val.find("=");
        size_t org = val.find(",");
        if (org == std::string::npos) {
            val = val.substr(pos + 1, val.size() - pos - 1);
        } else {
            val = val.substr(pos + 1, org - pos - 1);
        }
        if (val[val.size() - 1] == '\n') {
            val = val.substr(0, val.size() - 1);
        }
    }
    return val;
}

static void stringToVec(std::string str, std::vector<std::string>& vec)
{
    vec.clear();
    std::string tmp = "";
    while (!str.empty()) {
        std::string::size_type pos = str.find(",");
        if (pos == std::string::npos) {
            vec.push_back(str);
            break;
        }
        tmp = str.substr(0, pos);
        vec.push_back(tmp);
        str = str.substr(pos + 1);
    }
}
