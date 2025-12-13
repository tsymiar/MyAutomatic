#include "SeekTime.h"
#include "logging.h"

#ifdef NEW_PROJ
#ifdef SEEK_FRAME_HEAD
#undef SEEK_FRAME_HEAD
#define SEEK_FRAME_HEAD (0x1234cdef)
#endif
typedef TransProjFrame StruFrameHeader;
typedef uint32_t key_size_t;
#define FRAME_SIZE PROJECT_PAYLOAD_SIZE // Frame size remains unchanged
// The value written to the database only needs to be increasing and non-duplicated to be searchable
#define FRAME_TIME (header->majorHead.getSeekValue())
// #define FRAME_TIME (((DataXxx*)(header->data))->getSeekValue())
#else
typedef ProjectFrame StruFrameHeader;
typedef uint64_t key_size_t;
#define FRAME_SIZE (header->size)
#define FRAME_TIME (header->utctime)
#endif

SeekTime::SeekTime()
{
    m_dbMgr = NULL;
}

SeekTime::~SeekTime()
{
    if (!m_uninit) {
        uninit();
    }
}

int SeekTime::init(const std::string& fileName)
{
    LOG_INF("------- GIT_COMMIT=%s, BUILT_TIME=%s -------", _GIT_COMMIT, _BUILT_TIME);
    size_t pos = fileName.rfind('.');
    std::string dbName = fileName.substr(0, pos) + ".db";
    m_dbMgr = new(std::nothrow) SeekDbMgr();
    if (m_dbMgr == NULL) {
        LOG_ERR("new SeekDbMgr failed!");
        return -1;
    }
    int ret = m_dbMgr->connectDB(dbName);
    m_dbMgr->createTable();
    if (ret != 0) {
        LOG_ERR("connectDB(%s) failed(0x%08X).", dbName.c_str(), ret);
        return ret;
    }
    return 0;
}

void SeekTime::uninit()
{
    if (m_dbMgr != NULL) {
        if (m_dbMgr->isConnected()) {
            m_dbMgr->disconnectDB();
        }
        DELETE(m_dbMgr);
    }
    m_seekTimeMap.clear();
    m_uninit = true;
}

void SeekTime::setFilesTime(const std::vector<SelectTime>& times, const std::string& file)
{
    m_seekTimeMap.clear();
    if (!file.empty()) {
        m_seekTimeMap[file] = times;
    }
}

int SeekTime::findFileFragmentDetail(FILE* file, FileFrameData& startframe, FileFrameData& tailframe)
{
    uint64_t startpos = startframe.offset.head;
    uint64_t tailpos = startframe.offset.tail;
    uint64_t curpos = startframe.offset.average();
    uint64_t lastsize = tailpos - startpos;
    uint64_t cursize = tailpos - curpos;

    fseek(file, curpos, SEEK_SET);

    SeekTimeContent comidx{};
    memcpy(comidx.fileName, startframe.fileName, MAX_NAME_LEN);

    uint8_t buffer[m_windSize];
    uint32_t buffSize = m_windSize > cursize ? cursize : m_windSize;
    size_t bytes = 0;
    int count = 0;
    bool found = false;

    /* Binary search to find the left boundary position */
    if (startpos == tailpos) {
        LOG_INF("left terminal startpos=%lld, tailpos=%lld", startpos, tailpos);
        startframe.target.offset = startpos;

        comidx.value.size = 0;
        comidx.value.offset = startframe.target.offset;
        comidx.value.timestamp = startframe.target.timestamp;
        comidx.fileid = startframe.id;
        comidx.duration = -1; // For non-final results, set this field to -1 to distinguish from final results
        LOG_INF("left terminal target offset=%lld timestamp=%lld", startframe.target.offset, startframe.target.timestamp);
        if (tailpos != 0) {
            m_dbMgr->addIndexNoDuplex(&comidx);
        }
        found = true;
    }
    while (!found) {
        bytes = fread(buffer, 1, buffSize, file);
        LOG_DBG("000 fread bytes=%u buffer size=%u.", bytes, buffSize);
        for (size_t i = 0; i < (bytes - sizeof(key_size_t)); i++) {
            if (*(key_size_t*)(buffer + i) == SEEK_FRAME_HEAD) {
                StruFrameHeader* header = (StruFrameHeader*)(buffer + i);
                if (FRAME_TIME == startframe.target.timestamp) {
                    startframe.target.offset = curpos + i;
                    startframe.target.timestamp = FRAME_TIME;
                    LOG_INF("left found target offset=%lld timestamp=%lld", startframe.target.offset, startframe.target.timestamp);
                    found = true;
                } else if (FRAME_TIME < startframe.target.timestamp) {
                    startframe.offset.head = curpos + i; // record last search position
                    startframe.ftime.first = FRAME_TIME;
                } else {
                    startframe.offset.tail = curpos + i;
                    startframe.ftime.last = FRAME_TIME;
                }

                comidx.value.size = FRAME_SIZE;
                comidx.value.offset = curpos + i;
                comidx.value.timestamp = FRAME_TIME;
                comidx.fileid = startframe.id;
                comidx.duration = -1; // For non-final results, set this field to -1 to distinguish from final results
                m_dbMgr->addIndexNoDuplex(&comidx);
                break;
            }
        }

        if (found) {
            LOG_INF("findFileFragmentDetail %u last curPosition=%lld, timestamp at [%lld, %lld].", count, curpos, startframe.ftime.first, startframe.ftime.last);
            break;
        }

        /* Set next read size */
        curpos = startframe.offset.average();
        fseek(file, curpos, SEEK_SET);
        cursize = startframe.offset.tail - curpos;
        buffSize = m_windSize > cursize ? cursize : m_windSize;

        if (cursize < MIN_JUDGE_FRAME_SIZE || cursize == lastsize) {
            // Still not found this time, use previous result
            startframe.target.offset = startframe.offset.head;
            startframe.target.timestamp = startframe.ftime.first;
            LOG_WRN("left terminal target not found, return target offset=%lld timestamp=%lld cursize=%lld lastsize=%lld curSeekPos=%lld",
                startframe.target.offset, startframe.target.timestamp, cursize, lastsize, curpos);
            found = true;
        }
        lastsize = cursize;
        count++;
    }

    /* Binary search to find the right boundary position */
    startpos = tailframe.offset.head;
    tailpos = tailframe.offset.tail;
    curpos = tailframe.offset.average();
    cursize = tailpos - curpos;
    buffSize = m_windSize > cursize ? cursize : m_windSize;
    found = false;
    count = 0;
    bytes = 0;
    fseek(file, curpos, SEEK_SET);

    if (startpos == tailpos) {
        tailframe.target.offset = startpos;

        comidx.value.size = 0;
        comidx.value.offset = tailframe.target.offset;
        comidx.value.timestamp = tailframe.target.timestamp;
        comidx.fileid = tailframe.id;
        comidx.duration = -1; // For non-final results, set this parameter to -1 to distinguish from final results
        LOG_INF("right terminal target offset=%lld timestamp=%lld", tailframe.target.offset, tailframe.target.timestamp);
        if (tailpos != 0) {
            m_dbMgr->addIndexNoDuplex(&comidx);
        }
        found = true;
    }
    while (!found) {
        bytes = fread(buffer, 1, buffSize, file);
        LOG_DBG("fread %u bytes, buffsize=%u.", bytes, buffSize);
        for (size_t i = 0; i < bytes - sizeof(key_size_t); i++) {
            if (*(key_size_t*)(buffer + i) == SEEK_FRAME_HEAD) {
                StruFrameHeader* header = (StruFrameHeader*)(buffer + i);
                if (FRAME_TIME == tailframe.target.timestamp) {
                    tailframe.target.offset = curpos + i;
                    tailframe.target.timestamp = FRAME_TIME;
                    LOG_INF("right found target offset=%lld timestamp=%lld", tailframe.target.offset, tailframe.target.timestamp);
                    found = true;
                } else if (FRAME_TIME < tailframe.target.timestamp) {
                    tailframe.offset.head = curpos + i;
                    tailframe.ftime.first = FRAME_TIME;
                } else {
                    tailframe.offset.tail = curpos + i;
                    tailframe.ftime.last = FRAME_TIME;
                }

                comidx.value.size = FRAME_SIZE;
                comidx.value.offset = curpos + i;
                comidx.value.timestamp = FRAME_TIME;
                comidx.fileid = tailframe.id;
                comidx.duration = -1; // For non-final results, set this field to -1 to distinguish from final results
                m_dbMgr->addIndexNoDuplex(&comidx);
                break;
            }
        }

        if (found) {
            break;
        }

        /* Set next read size */
        curpos = tailframe.offset.average();
        cursize = tailframe.offset.tail - curpos;
        buffSize = m_windSize > cursize ? cursize : m_windSize;
        fseek(file, curpos, SEEK_SET);

        if (cursize < MIN_JUDGE_FRAME_SIZE || lastsize == cursize) {
            tailframe.target.offset = tailframe.offset.tail;
            tailframe.target.timestamp = tailframe.ftime.last;
            LOG_WRN("left terminal target not found, return target offset=%lld timestamp=%lld cursize=%lld lastsize=%lld curpos=%lld",
                tailframe.target.offset, tailframe.target.timestamp, cursize, lastsize, curpos);
            found = true;
        }
        lastsize = cursize;
        count++;
    }
    return (found ? 0 : -1);
}

/* Find first frame information of the file */
bool getFirstFrame(FILE* file, uint32_t winsize, SeekTimeContent& headIdx, uint32_t offset)
{
    uint64_t position = 0;
    fseek(file, 0, SEEK_END);
    int64_t filesize = ftell(file);
    fseek(file, offset, SEEK_SET);
    uint8_t buffer[winsize];
    uint32_t buffSize = winsize < filesize ? winsize : filesize;
    size_t bytes = 0;
    int current = 0;
    bool find = false;
    while ((bytes = fread(buffer, 1, buffSize, file)) > 0) {
        for (size_t i = 0; i < bytes - sizeof(key_size_t); i++) {
            if (*(key_size_t*)(buffer + i) == SEEK_FRAME_HEAD) {
                StruFrameHeader* header = (StruFrameHeader*)(buffer + i);
                headIdx.value.timestamp = FRAME_TIME;
                headIdx.duration = -1;
                headIdx.value.offset = offset + position + i;
                headIdx.value.size = FRAME_SIZE;
                uint32_t sec = headIdx.value.timestamp >> 32;
                uint32_t tick = headIdx.value.timestamp & 0xffffffff;
                uint64_t value = sec * 1000000UL + tick * 20 / 1000;
                LOG_INF("found file frame head (0x%08x) with timestamp: %llu, mintime=%llu, offset=0x%x, size=0x%x in bytes=%u",
                    header->syncHead, headIdx.value.timestamp, value, headIdx.value.offset, headIdx.value.size, bytes);
                find = true;
                break;
            }
        }
        if (find) {
            break;
        }
        position += (bytes - sizeof(key_size_t));
        if ((filesize - position) <= sizeof(key_size_t)) {
            LOG_WRN("expected frame structure is not found in this file. filesize=%lld.", filesize);
            break;
        }
        current++;
        fseek(file, position, SEEK_SET);
    }
    return find;
}

/* Find last frame information of the file */
bool getTailFrame(FILE* file, uint32_t winsize, SeekTimeContent& tailIdx)
{
    fseek(file, 0, SEEK_END);
    int64_t filesize = ftell(file);
    fseek(file, 0, SEEK_SET);
    uint8_t buffer[winsize];
    uint64_t position = filesize - winsize > 0 ? (filesize - winsize) : 0;
    uint64_t buffSize = position > 0 ? winsize : filesize;
    bool find = false;
    do {
        fseek(file, position, SEEK_SET);
        size_t bytes = fread(buffer, 1, buffSize, file);
        if (bytes > 0) {
            for (size_t i = bytes - sizeof(key_size_t); i > 0; i--) {
                if (*(key_size_t*)(buffer + i) == SEEK_FRAME_HEAD) {
                    StruFrameHeader* header = (StruFrameHeader*)(buffer + i);
                    tailIdx.value.timestamp = FRAME_TIME;
                    tailIdx.duration = -1;
                    tailIdx.value.offset = position + i;
                    tailIdx.value.size = FRAME_SIZE;
                    uint32_t sec = tailIdx.value.timestamp >> 32;
                    uint32_t tick = tailIdx.value.timestamp & 0xffffffff;
                    uint64_t value = sec * 1000000UL + tick * 20 / 1000;
                    LOG_INF("found file frame tail with timestamp: %lld, maxtime=%llu, offset=0x%x, size=0x%x in bytes=%u",
                        tailIdx.value.timestamp, value, tailIdx.value.offset, tailIdx.value.size, bytes);
                    find = true;
                    break;
                }
            }
        } else {
            LOG_WRN("fread failed expected frame structure is not found in this file. filesize=%lld.", filesize);
            break;
        }
        if (find) {
            break;
        }
        if (position >= bytes) {
            position = position - bytes;
            // bytes unchanged
        } else {
            buffSize = position;
            position = 0;
        }
    } while (buffSize > 0);
    return find;
}

int SeekTime::seekFileDataTime(uint32_t duration, std::vector<SeekTimeContent>& fileinfos, uint32_t offset)
{
    for (std::pair<std::string, std::vector<SelectTime>> vecFileTime : m_seekTimeMap) {
        int ret = 0;
        uint64_t beginTime = getUsecTime();
        m_pfile = fopen(vecFileTime.first.c_str(), "rb");
        if (!m_pfile) {
            LOG_ERR("fopen(%s) failed: %s!", vecFileTime.first.c_str(), strerror(errno));
            return -1;
        }
        for (SelectTime& seekTime : vecFileTime.second) {
            SeekTimeContent fileinfo{};
            snprintf(fileinfo.fileName, MAX_NAME_LEN, "%s", vecFileTime.first.c_str());
            LOG_INF("'%s', selecting time=%lld ...", fileinfo.fileName, seekTime.average());

            // First check the database TblFileIdMapping for a record of this file; if not found, insert a record
            uint32_t fileId = 0;
            FileFrameData startframe{};
            FileFrameData tailframe{};
            snprintf(startframe.fileName, MAX_NAME_LEN, "%s", vecFileTime.first.c_str());
            snprintf(tailframe.fileName, MAX_NAME_LEN, "%s", vecFileTime.first.c_str());
            startframe.target.timestamp = seekTime.first;
            tailframe.target.timestamp = seekTime.last;
            FileFrameData fileMapping{};
            if (m_dbMgr->queryFileIdbyName(vecFileTime.first, fileId) != 0) {
                SeekTimeContent headIdx{};
                SeekTimeContent tailIdx{};
                snprintf(fileMapping.fileName, MAX_NAME_LEN, "%s", vecFileTime.first.c_str());
                bool find = getFirstFrame(m_pfile, m_windSize, headIdx, offset);
                if (find) {
                    snprintf(headIdx.fileName, MAX_NAME_LEN, "%s", vecFileTime.first.c_str());
                    fileMapping.offset.head = headIdx.value.offset;
                    fileMapping.ftime.first = headIdx.value.timestamp;
                    LOG_INF("getFirstFrame offset=%lld len=%lld timestamp=%lld fileid=%u", headIdx.value.offset, headIdx.value.size, headIdx.value.timestamp, headIdx.fileid);
                    ret = m_dbMgr->insertFileIdbyName(&fileMapping);
                    if (ret != 0) {
                        LOG_ERR("insertFileIdbyName() failed(%d)!", ret);
                    }
                    /* Insert the first and last frame information of the file into TblSeekTimeIndex */
                    ret = m_dbMgr->addIndexNoDuplex(&headIdx);
                    if (ret != 0) {
                        LOG_ERR("addIndexNoDuplex() headIdx failed!");
                    }
                }
                find = getTailFrame(m_pfile, m_windSize, tailIdx);
                if (find) {
                    snprintf(tailIdx.fileName, MAX_NAME_LEN, "%s", vecFileTime.first.c_str());
                    /* Insert file information into TblFileIdMapping table */
                    fileMapping.offset.tail = tailIdx.value.offset;
                    fileMapping.ftime.last = tailIdx.value.timestamp;
                    ret = m_dbMgr->insertFileIdbyName(&fileMapping);
                    if (ret != 0) {
                        LOG_ERR("insertFileIdbyName() fileMapping failed!");
                    }
                    ret = m_dbMgr->addIndexNoDuplex(&tailIdx);
                    if (ret != 0) {
                        LOG_ERR("addIndexNoDuplex() tailIdx failed!");
                    }
                }
                fseek(m_pfile, 0, SEEK_SET);
            }
            /* If TblFileIdMapping has information for this file, query frame info in TblSeekTimeIndex */
            else {
                /* Database already has a record for the target segment, continue to next target segment */
                fileinfo.fileid = fileId;
                if (m_dbMgr->getTargetFragmentByTime(seekTime.average(), duration, &fileinfo) == 0) {
                    if (fileinfo.value.size > 0) {
                        fileinfos.emplace_back(fileinfo);
                        LOG_INF("getTargetFragmentByTime success (target offset=%lld len=%lld, target timestamp=%lld, duration=%d).",
                            fileinfo.value.offset, fileinfo.value.size, fileinfo.value.timestamp, fileinfo.duration);
                    } else {
                        LOG_WRN("result is not nomal (target offset=%lld len=%lld, target timestamp=%lld, duration=%d).",
                            fileinfo.value.offset, fileinfo.value.size, fileinfo.value.timestamp, fileinfo.duration);
                    }
                    continue;
                }
            }

            /* If not found exactly in database, find the smallest range for start and end times */
            ret = m_dbMgr->getTimeOffsetByFileName(vecFileTime.first, seekTime, startframe, tailframe);
            if (ret != 0) {
                LOG_WRN("can not get TimeOffset in dataBase , target time is out of range, quit!");
                continue;
            }

            /* Based on the smallest range from database, use binary search within the range to find left and right boundary targets */
            LOG_INF("before findFileFragmentDetail: Offset left[%lld, %lld], right[%lld, %lld], Time left[%lld, %lld] right[%lld, %lld].",
                startframe.offset.head, startframe.offset.tail, tailframe.offset.head, tailframe.offset.tail,
                startframe.ftime.first, startframe.ftime.last, tailframe.ftime.first, tailframe.ftime.last);
            ret = findFileFragmentDetail(m_pfile, startframe, tailframe);
            if (ret != 0) {
                LOG_WRN("findFileFragmentDetail failed, return default info(target offset=%lld len=%lld).", startframe.target.offset, (tailframe.target.offset - startframe.target.offset));
                fileinfo.found = false;
            } else {
                fileinfo.fileid = startframe.id;
                fileinfo.value.size =
                    (tailframe.offset.tail > tailframe.target.offset ? tailframe.offset.tail : tailframe.target.offset)
                    - (startframe.target.offset < startframe.offset.head ? startframe.target.offset : startframe.offset.head);
                fileinfo.value.offset = startframe.target.offset;
                fileinfo.value.timestamp = seekTime.average();
                fileinfo.duration = duration;
                if (fileinfo.value.size > 0) {
                    fileinfos.emplace_back(fileinfo);
                    LOG_INF("findFileFragmentDetail success (target offset=%lld len=%lld).", startframe.target.offset, fileinfo.value.size);
                } else {
                    LOG_WRN("result is not nomal (target offset=%lld len=%lld).", startframe.target.offset, fileinfo.value.size);
                }
                ret = m_dbMgr->addIndexNoDuplex(&fileinfo);
                if (ret != 0) {
                    LOG_ERR("addIndexNoDuplex() fileinfo failed!");
                }
            }
        }
        if (m_pfile != NULL) {
            fclose(m_pfile);
        }
        LOG_INF("------- Parse file(%s) frame cost %.3fs -------", vecFileTime.first.c_str(), (getUsecTime() - beginTime) * 1.f / 1000000.0f);
    }
    return 0;
}

int SeekTime::getFileTime(const std::string& sfile, SelectTime& time, uint32_t offset)
{
    if (m_dbMgr == NULL) {
        this->init(sfile);
    }
    if (0 != m_dbMgr->queryFileTime(sfile, time)) {
        FILE* file = fopen(sfile.c_str(), "rb");
        if (!file) {
            LOG_ERR("fopen(%s) failed: %s!", sfile.c_str(), strerror(errno));
            return -1;
        }
        SeekTimeContent frameIdx{};
        if (getFirstFrame(file, m_windSize, frameIdx, offset)) {
            time.first = frameIdx.value.timestamp;
        }
        if (getTailFrame(file, m_windSize, frameIdx)) {
            time.last = frameIdx.value.timestamp;
        }
    }
    LOG_INF("------- Parse file(%s) time=(first=%llu,last=%llu) -------", sfile.c_str(), time.first, time.last);
    return 0;
}

SelectTime SeekTime::getTimeDuration(std::vector<std::string> files)
{
    if (m_timeDuration.first == 0 || m_timeDuration.last == 0) {
        for (auto file : files) {
            SelectTime time{};
            if (getFileTime(file, time) < 0) {
                break;
            }
            if (m_timeDuration.first == 0 || (time.first < m_timeDuration.first && time.first > 0)) {
                m_timeDuration.first = time.first;
            }
            if (m_timeDuration.last == 0 || (time.last > m_timeDuration.last && m_timeDuration.last > 0)) {
                m_timeDuration.last = time.last;
            }
        }
    }
    return m_timeDuration;
}
