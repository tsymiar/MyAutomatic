#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <sys/time.h>

// Shared helpers (getFileAsCstring / getVariable ...) live in ../../include/Utils.h
#include "../include/Utils.h"

#ifndef MAX_NAME_LEN
#define MAX_NAME_LEN 128
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
    char fileName[MAX_NAME_LEN];
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
