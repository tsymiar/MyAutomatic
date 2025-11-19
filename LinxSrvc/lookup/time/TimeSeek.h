#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include "common.h"

class TimeDbMgr; // Forward declaration

class TimeSeek {
public:
    TimeSeek();
    virtual ~TimeSeek();

    int init(const std::string& dbName = "./seekTime.db");
    void uninit();
    int seekFileDataTime(std::vector<SeekTimeContent>& fileinfos);
    void setFilesTime(const std::vector<SelectTime>& times, const std::string& file);

private:
    SeekTimeContent parseFileFrame(SelectOffset position, int64_t timestamp, uint64_t minpos = 0);
    std::vector<SeekTimeContent> sortFramebyTime(SelectOffset selectOffset, uint64_t timestamp, SeekTimeContent& fileinfo);

private:
    std::map<std::string, std::vector<SelectTime> > m_seekTimeMap{};
    TimeDbMgr* m_dbMgr = NULL;
    FileTimeDetails m_timeDetail{ {1, 0x100000000}, {0, 0x100000000} };
    uint32_t m_windSize = 0x100000;
    uint32_t m_maxFrameSize = 0x100000;
    FILE* m_file = NULL;
    bool m_hasDeinit = false;
    std::string m_filename{};
};
