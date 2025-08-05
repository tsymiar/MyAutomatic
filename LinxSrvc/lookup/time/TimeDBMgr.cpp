#include "TimeDBMgr.h"
#include "Common.h"

#define TABLE_FILE_MAPPING "TblFileMapping"
#define TABLE_SEEK_TIME "TblSeekTime"
#define TABLE_INDEX "id"
#define FIELD_TIME "Time"
#define FIELD_OFFSET "Offset"
#define FIELD_SIZE "Size"
#define FIELD_FILEID "FileId"
#define FIELD_FILENAME "FileName"
#define FIELD_FIRST_TIME "FirstTime"
#define FIELD_LAST_TIME "LastTime"
#define FIELD_FIRST_OFFSET "FirstOffset"
#define FIELD_LAST_OFFSET "LastOffset"
#define FIELD_FILESIGN "FileSign"

const uint32_t SQL_LEN = 1024;

int TimeDBMgr::connect(const std::string& db)
{
    // connect to the database
    printf("Connecting to database: %s\n", db.c_str());
    m_connected = true; // Simulate a successful connection
    return 0;
}

bool TimeDBMgr::connected() const
{
    // check if connected to the database
    return false;
}

void TimeDBMgr::disconnect()
{
    // disconnect from the database
    printf("Disconnecting database\n");
}

void TimeDBMgr::create()
{
    char sql[SQL_LEN];
    sprintf(sql, "create table %s(id integer primary key autoincrement, %s integer, %s integer, %s integer, %s integer)",
        TABLE_SEEK_TIME, FIELD_TIME, FIELD_OFFSET, FIELD_SIZE, FIELD_FILEID);
    printf("sql:[%s](%d)\n", sql, __LINE__);
    sprintf(sql, "create index if not exists id on %s(%s)", TABLE_SEEK_TIME, FIELD_TIME);
    printf("sql:[%s](%d)\n", sql, __LINE__);
    sprintf(sql, "create table %s(id integer primary key autoincrement, %s char(128), %s integer, %s integer, %s integer, %s integer, %s char(16))",
        TABLE_FILE_MAPPING, FIELD_FILENAME,
        FIELD_FIRST_OFFSET, FIELD_LAST_OFFSET,
        FIELD_FIRST_TIME, FIELD_LAST_TIME,
        FIELD_FILESIGN);
    printf("sql:[%s](%d)\n", sql, __LINE__);
}

int TimeDBMgr::queryTimeOffset(SelectValue seek, std::vector<SeekTimeValue>& seekOffsets, const std::string& file, int fileid)
{
    m_filename = file;
    if (!m_connected) {
        ERROR("Database NOT connected!\n");
        return -1;
    }
    int fileid1 = 0; // This should be replaced with actual logic to get fileid based on filename
    getFileIdbyName(m_filename, fileid1);
    if (fileid != 0)
        fileid1 = fileid;
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "select %s,%s,%s,%s from %s where %s>=%lu and %s<=%lu and %s=%d ",
        FIELD_TIME,
        FIELD_OFFSET,
        FIELD_SIZE,
        FIELD_FILEID,
        TABLE_SEEK_TIME,
        FIELD_TIME,
        seek.first,
        FIELD_TIME,
        seek.last,
        FIELD_FILEID,
        fileid1);
    printf("sql:[%s](%d)\n", sql, __LINE__);

    return 0;
}

int TimeDBMgr::queryTimeDetail(FileTimeDetails& detail)
{
    detail.time.first = detail.time.last = -1;
    detail.offset.first = detail.offset.last = 0;
    std::string sql = "select " FIELD_FIRST_OFFSET "," FIELD_LAST_OFFSET "," FIELD_FIRST_TIME "," FIELD_LAST_TIME
        " from " TABLE_FILE_MAPPING
        " limit 1;";
    printf("sql:[%s](%d)\n", sql.c_str(), __LINE__);
    return 0;
}

void TimeDBMgr::setDetailByFileName(const std::string& fileName, const FileTimeDetails& detail)
{
    std::string sql = "UPDATE " TABLE_FILE_MAPPING " SET " FIELD_FIRST_OFFSET "=" + std::to_string(detail.offset.first) +
        ", " FIELD_LAST_OFFSET "=" + std::to_string(detail.offset.last) +
        ", " FIELD_FIRST_TIME "=" + std::to_string(detail.time.first) +
        ", " FIELD_LAST_TIME "=" + std::to_string(detail.time.last);
    printf("sql:[%s](%d)\n", sql.c_str(), __LINE__);
}

void TimeDBMgr::insertContentNoDuplex(SeekTimeContent* content)
{
    int fileid = 0;
    if (getFileIdbyName(m_filename, fileid) < 0) {
        printf("File ID is 0, cannot insert content!\n");
        std::string sql = "INSERT INTO " TABLE_FILE_MAPPING " (" FIELD_FILENAME "," FIELD_FIRST_OFFSET "," FIELD_LAST_OFFSET "," FIELD_FIRST_TIME "," FIELD_LAST_TIME
            ") SELECT '" +
            m_filename + "',0,0,-1,-1 WHERE NOT EXISTS (SELECT 1 FROM " TABLE_FILE_MAPPING " WHERE " FIELD_FILENAME " = '" +
            m_filename + "');";
        printf("sql:[%s](%d)\n", sql.c_str(), __LINE__);
    }
    char sql[SQL_LEN] = { 0 };
    std::string ssql = "";
    sprintf(sql, "insert into %s(%s,%s,%s,%s)",
        TABLE_SEEK_TIME,
        FIELD_TIME,
        FIELD_OFFSET,
        FIELD_SIZE,
        FIELD_FILEID);
    ssql += sql;

    sprintf(sql, "SELECT %lu,%lu,%lu,%d WHERE NOT EXISTS",
        content->value.timestamp,
        content->value.offset,
        content->value.size,
        fileid);
    content->fileid = fileid;
    ssql += sql;

    sprintf(sql, "(SELECT 1 FROM %s WHERE %s = %lu);",
        TABLE_SEEK_TIME,
        FIELD_OFFSET,
        content->value.offset);
    ssql += sql;
    printf("sql:[%s](%d)\n", ssql.c_str(), __LINE__);
}

int TimeDBMgr::getFileIdbyName(const std::string& filename, int& fileid)
{
    // Assuming fileid is obtained from the filename
    if (m_filename.empty()) {
        ERROR("Filename is NULL!\n");
        return -1;
    }
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "select id from %s where %s='%s' ",
        TABLE_FILE_MAPPING,
        FIELD_FILENAME,
        filename.c_str());
    printf("sql:[%s](%d)\n", sql, __LINE__);
    fileid = 0;
    return 0;
}
