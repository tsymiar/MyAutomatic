#pragma once
#include <string>
#include <vector>
#include "Common.h"

class TimeDBMgr {
public:
    int connect(const std::string& db);
    void create();
    bool connected() const;
    void disconnect();
    int queryTimeOffset(SelectValue seek, std::vector<SeekTimeValue>& offsets, const std::string& file, int fileid = 0);
    int queryTimeDetail(FileTimeDetails& detail);
    void setDetailByFileName(const std::string& fileName, const FileTimeDetails& detail);
    void insertContentNoDuplex(const SeekTimeContent* content);
private:
    std::string m_filename{};
    bool m_connected = false;
private:
    int getFileIdbyName(const std::string& filename, int& fileid);
};
