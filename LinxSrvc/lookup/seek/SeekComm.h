#pragma once
#include <stdint.h>
#include <cstring>
#include "common.h"

typedef SelectValue SelectTime;
typedef SelectValue SelectOffset;
typedef SelectValue SelectFileTime;

struct SelectFileOffset {
    int64_t head;
    int64_t tail;
    const int64_t average()
    {
        return (this->tail + this->head) / 2;
    }
    const int64_t length()
    {
        return (this->tail - this->head);
    }
    bool operator==(const SelectFileOffset& value) const
    {
        return ((head == value.head) && (tail == value.tail));
    }
    SelectFileOffset()
    {
        head = tail = 0;
    }
};

struct FileFrameData {
    uint32_t id;
    char fileName[MAX_NAME_LEN];
    struct {
        int64_t timestamp;
        int64_t offset;
    } target;
    SelectFileTime ftime;
    SelectFileOffset offset;
    char fileSign[128];
    FileFrameData()
    {
        id = target.timestamp = target.offset = 0;
        memset(fileSign, 0, sizeof(fileSign));
    }
};
