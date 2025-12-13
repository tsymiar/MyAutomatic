#include "TimeSeek.h"
#include <algorithm>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <fstream>
#include <iostream>
#include "common.h"
#include "logging.h"
#include "TimeDbMgr.h"

using namespace std;

TimeSeek::TimeSeek() : m_dbMgr(NULL)
{ }

TimeSeek::~TimeSeek()
{
    if (!m_hasDeinit) {
        uninit();
    }
}

int TimeSeek::init(const std::string& dbName)
{
    if (m_dbMgr != NULL) {
        delete m_dbMgr;
        m_dbMgr = NULL;
    }
    m_dbMgr = new(std::nothrow) TimeDbMgr();
    if (m_dbMgr == NULL || m_dbMgr->connect(dbName) != 0) {
        LOG_ERR("connect(%s) failed: %s.", dbName.c_str(), strerror(errno));
        return -1;
    }
    m_dbMgr->create();
    return 0;
}

void TimeSeek::uninit()
{
    if (m_dbMgr != NULL) {
        if (m_dbMgr->connected()) {
            m_dbMgr->disconnect();
        }
        delete m_dbMgr;
        m_dbMgr = NULL;
    }
    m_seekTimeMap.clear();
    m_hasDeinit = true;
}

void TimeSeek::setFilesTime(const std::vector<SelectTime>& times, const std::string& file)
{
    if (!file.empty()) {
        m_seekTimeMap[file] = times;
    }
}

static SeekTimeContent parseFrameFirst(FILE* file, TimeDbMgr* dbMgr, int position = 0, long int targetHeader = CONST_FRAME_HEAD, uint32_t windSize = 1024)
{
    if (file == NULL) {
        return {};
    }
    SeekTimeContent comidx{};
    fseek(file, 0, SEEK_END);
    uint64_t fileSize = ftell(file);
    fseek(file, position, SEEK_SET);
    uint8_t buffer[windSize]; // 1MB
    memset(buffer, 0, sizeof(buffer));
    size_t bytes = 0;
    int current = 0;
    bool once = true;
    while ((bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        for (size_t i = 0; i <= bytes - sizeof(uint64_t); i++) {
            if (*(uint64_t*)(buffer + i) == targetHeader) {
                UserFileFrameHeader* header = (UserFileFrameHeader*)(buffer + i);
                if (header->timestamp == 0) {
                    printf("timestamp format error!\n");
                    break;
                }
                comidx.value.timestamp = header->timestamp;
                comidx.value.offset = position + i;
                comidx.value.size = header->len;
                dbMgr->insertContentNoDuplex(&comidx);
                if (once) {
                    fseek(file, 0, SEEK_SET);
                    printf("parseFileFrame: found frame head with timestamp: %lu, offset=%lu, size=%lu\n", comidx.value.timestamp, comidx.value.offset, comidx.value.size);
                    return comidx;
                }
            }
        }
        position += (bytes - sizeof(uint64_t));
        if ((fileSize - position) <= sizeof(uint64_t)) {
            break;
        }
        current++;
        printf("--- %d last 0x%x parsed, seek at 0x%x.\n", current, windSize, position);
        fseek(file, position, SEEK_SET);
    }
    fseek(file, 0, SEEK_SET);
    return comidx;
}

static SeekTimeContent parseFrameBack(FILE* file, uint32_t windSize)
{
    if (file == NULL) {
        return {};
    }
    fseek(file, 0, SEEK_END);
    uint64_t fileSize = ftell(file);
    uint64_t curpos = fileSize - windSize;
    uint32_t size = fileSize > windSize ? windSize : fileSize;
    uint8_t buff[size];
    do {
        size_t bytes = 0;
        fseek(file, curpos, SEEK_SET);
        while ((bytes = fread(buff, 1, size, file)) > 0) {
            for (size_t i = bytes - sizeof(uint64_t); i > 0; i--) {
                if (*(uint64_t*)(buff + i) == CONST_FRAME_HEAD) {
                    UserFileFrameHeader* header = (UserFileFrameHeader*)(buff + i);
                    if (header->timestamp == 0) {
                        printf("timestamp format error!\n");
                        break;
                    }
                    SeekTimeContent comidx{};
                    comidx.value.offset = curpos + i;
                    comidx.value.size = header->len;
                    comidx.value.timestamp = header->timestamp;
                    printf("parseFrameBack: found frame head from tail with timestamp: %lu, offset=%lu, size=%lu\n", comidx.value.timestamp, comidx.value.offset, comidx.value.size);
                    return comidx;
                }
            }
        }
        curpos -= size;
    } while (curpos > 0);
    return {};
}

SeekTimeContent TimeSeek::parseFileFrame(SelectOffset position, int64_t timestamp, uint64_t minpos)
{
    if (m_file == NULL) {
        return {};
    }
    uint64_t firstpos = position.first;
    uint64_t average = position.average();
    uint64_t curpos = average;
    fseek(m_file, average, SEEK_SET);
    uint8_t buffer[m_windSize]; // 1MB
    memset(buffer, 0, sizeof(buffer));
    size_t bytes = 0;
    int current = 0;
    int equal_cnt = 0;
    bool firstSeek = true;
    SelectOffset lastpos = position;
    SeekTimeContent comidx{};
    SeekTimeValue firstValue{};
    SeekTimeValue lastValue{};
    SeekTimeValue calcValue{};
    while ((bytes = fread(buffer, 1, sizeof(buffer), m_file)) > 0) {
        for (size_t i = 0; i <= bytes - sizeof(uint64_t); i++) {
            size_t idx = i;
            if (*(uint64_t*)(buffer + idx) == CONST_FRAME_HEAD) {
                UserFileFrameHeader* header = (UserFileFrameHeader*)(buffer + idx);
                if (header->timestamp > timestamp) {
                    position.last = curpos + idx;
                } else {
                    position.first = curpos + idx;
                }
                comidx.value.offset = curpos + idx;
                comidx.value.size = header->len;
                comidx.value.timestamp = header->timestamp;
                m_dbMgr->insertContentNoDuplex(&comidx);
                curpos = position.average();
                break;
            }
        }
        if (position == lastpos) {
            if (firstSeek) {
                firstValue = lastValue;
            }
            equal_cnt++;
        }
        lastpos = position;
        if (equal_cnt == 1 && comidx.value.timestamp > timestamp && firstSeek) {
            position.first = curpos = minpos;
            position.last = firstpos;
            firstSeek = false;
            equal_cnt = 0;
        } else {
            current++;
            printf("--- read 0x%xB at %d, offset [%lu, %lu], time=%lu.\n", m_windSize, current, position.first, position.last, comidx.value.timestamp);
        }
        if ((!firstSeek && equal_cnt >= 1) || (position.last - position.first <= MIN_FRAME_SIZE)) {
            if (llabs(lastValue.timestamp - timestamp) > llabs(firstValue.timestamp - timestamp))
                comidx.value = firstValue;
            else
                comidx.value = lastValue;
            if (llabs(comidx.value.timestamp - timestamp) > llabs(calcValue.timestamp - timestamp))
                comidx.value = calcValue;
            break;
        }
        fseek(m_file, curpos, SEEK_SET);
        if (llabs(lastValue.timestamp - timestamp) < llabs(calcValue.timestamp - timestamp))
            calcValue = lastValue;
        lastValue = comidx.value;
    }
    comidx.found = true;
    return comidx;
}

bool compareByTime(const SeekTimeContent& x, const SeekTimeContent& y)
{
    return x.value.timestamp < y.value.timestamp;
}

bool getTimefromDatabase(TimeDbMgr* dbMgr, SelectValue selectValue, SeekTimeContent& fileinfo, std::vector<SeekTimeContent>& fileinfos)
{
    std::vector<SeekTimeValue> seekOffsets{};
    if (dbMgr->queryTimeOffset(selectValue, seekOffsets, fileinfo.fileName) == 0) {
        size_t size = seekOffsets.size();
        if (size >= 1) {
            size_t v = 1;
            uint64_t delta = 0;
            for (size_t i = 0; i < size; i++) {
                if (llabs(selectValue.average() - seekOffsets[i].timestamp) < delta && i > 0) {
                    v = i;
                }
                delta = llabs(selectValue.average() - seekOffsets[i].timestamp);
                SeekTimeValue value;
                value.timestamp = seekOffsets[i].timestamp;
                value.offset = seekOffsets[i].offset;
                value.size = seekOffsets[i].size;
                printf("found value=[%lu, %lu, %lu]\n", value.timestamp, value.offset, value.size);
            }
            for (size_t i = v - 1; i < size; i++) {
                fileinfo.totalSize += seekOffsets[i].size;
            }
            fileinfo.found = true;
            fileinfo.value = seekOffsets[v - 1];
            fileinfo.param = selectValue.average();
            fileinfos.emplace_back(fileinfo);
        } else {
            LOG_ERR("Not find file time from %lu to %lu.", selectValue.first, selectValue.last);
        }
        return true;
    }
    return false;
}

int TimeSeek::seekFileDataTime(std::vector<SeekTimeContent>& fileinfos)
{
    int status = 0;
    uint64_t beginTime = getUsecTime();
    for (auto it : m_seekTimeMap) {
        for (auto at : it.second) {
            SeekTimeContent fileinfo{};
            snprintf(fileinfo.fileName, MAX_NAME_LEN, "%s", it.first.c_str());
            printf("--- '%s', selecting time=%lu\n", fileinfo.fileName, at.average());
            // check first if time is in database
            if (getTimefromDatabase(m_dbMgr, at, fileinfo, fileinfos)) {
                continue;
            }
            m_file = fopen(it.first.c_str(), "rb");
            if (!m_file) {
                perror("fopen");
                return -1;
            }
            // get first and last frame time from file
            SeekTimeContent head{};
            if (m_dbMgr->queryTimeDetail(m_timeDetail) != 0 ||
                m_timeDetail.time.first == -1 || m_timeDetail.time.last == -1) {
                head = parseFrameFirst(m_file, m_dbMgr, 0, CONST_FRAME_HEAD, m_windSize);
                m_timeDetail.time.first = head.value.timestamp;
                m_timeDetail.offset.first = head.value.offset;
                SeekTimeContent tail = parseFrameBack(m_file, m_windSize);
                m_timeDetail.time.last = tail.value.timestamp;
                m_timeDetail.offset.last = tail.value.offset;
                m_dbMgr->setDetailByFileName(it.first, m_timeDetail);
            }
            printf("--- data frame time=[%lu, %lu], offset=[%lu, %lu]\n", m_timeDetail.time.first, m_timeDetail.time.last, m_timeDetail.offset.first, m_timeDetail.offset.last);
            if (at.average() < m_timeDetail.time.first || at.average() > m_timeDetail.time.last) {
                printf("Not found timestamp %lu\n", at.average());
                fileinfo.found = false;
                fileinfo.param = at.average();
                fileinfos.emplace_back(fileinfo);
                continue;
            }
            FileTimeDetails timeOffset = m_timeDetail;
            // search time by earliest and latest frame time
            SelectOffset selectOffset{};
            if (at.average() < timeOffset.time.average()) {
                selectOffset.first = timeOffset.offset.first;
                selectOffset.last = timeOffset.offset.average();
            } else {
                selectOffset.first = timeOffset.offset.average();
                selectOffset.last = timeOffset.offset.last;
            }
            printf("--- select offset at [%lu, %lu]\n", selectOffset.first, selectOffset.last);
            if (m_timeDetail.offset.first == m_timeDetail.offset.last) {
                fileinfo = head;
                if (head.value.timestamp == at.average()) {
                    fileinfo.found = true;
                }
            } else {
                fileinfo = parseFileFrame(selectOffset, at.average(), m_timeDetail.offset.first);
            }
            if (fileinfo.found) {
                snprintf(fileinfo.fileName, MAX_NAME_LEN, "%s", it.first.c_str());
                fileinfo.param = at.average();
                fileinfos.emplace_back(fileinfo);
            } else {
                LOG_ERR("Not found timestamp: %lu from '%s'.", at.average(), it.first.c_str());
            }
            if (m_file != NULL) {
                fclose(m_file);
            }
        }
    }
    printf("--- Parse file frame cost %.3fs\n", (getUsecTime() - beginTime) * 1.f / 1000000.0f);
    return status;
}

std::vector<SeekTimeContent> TimeSeek::sortFramebyTime(SelectOffset selectOffset, uint64_t timestamp, SeekTimeContent& fileinfo)
{
    std::vector<SeekTimeContent> seekTimes{};
    {
        if (m_file == NULL) {
            return {};
        }
        SeekTimeContent comidx{};
        fseek(m_file, 0, SEEK_END);
        uint64_t fileSize = ftell(m_file);
        uint64_t position = selectOffset.average();
        fseek(m_file, position, SEEK_SET);
        uint8_t frmBuff[0x100000]; // 1MB
        memset(frmBuff, 0, sizeof(frmBuff));
        size_t bytes = 0;
        int current = 0;
        bool once = true;
        while ((bytes = fread(frmBuff, 1, sizeof(frmBuff), m_file)) > 0) {
            for (size_t i = 0; i <= bytes - sizeof(uint64_t); i++) {
                if (*(uint64_t*)(frmBuff + i) == CONST_FRAME_HEAD) {
                    UserFileFrameHeader* header = (UserFileFrameHeader*)(frmBuff + i);
                    comidx.value.offset = position + i;
                    comidx.value.size = header->len;
                    comidx.value.timestamp = header->timestamp;
                    m_dbMgr->insertContentNoDuplex(&comidx);
                    if (once) {
                        fseek(m_file, 0, SEEK_SET);
                        printf("parseFileFrame: found frame head with timestamp: %lu, offset=%lu, size=%lu\n", comidx.value.timestamp, comidx.value.offset, comidx.value.size);
                        seekTimes.push_back(comidx);
                    }
                }
            }
            position += (bytes - sizeof(uint64_t));
            if ((fileSize - position) <= sizeof(uint64_t)) {
                break;
            }
            current++;
            printf("--- %d last 1MB parsed, seek at 0x%lx.\n", current, position);
            fseek(m_file, position, SEEK_SET);
        }
        fseek(m_file, 0, SEEK_SET);
    }
    if (seekTimes.size() > 1) {
        std::sort(seekTimes.begin(), seekTimes.end(), compareByTime);
        {
            for (auto timeinfo : seekTimes) {
                if (timestamp == timeinfo.value.timestamp) {
                    fileinfo = timeinfo;
                    break;
                } else if (timestamp < timeinfo.value.timestamp) {
                    continue;
                } else {
                }
            }
            printf("Found '%s' timestamp: %lu, frame offset=%lu, size=%lu\n", fileinfo.fileName, timestamp, fileinfo.value.offset, fileinfo.value.size);
        }
    } else if (seekTimes.size() == 1) {
        fileinfo = seekTimes[0];
    } else {
        LOG_ERR("Not found timestamp: %lu from '%s'.", timestamp, fileinfo.fileName);
    }
    return seekTimes;
}
