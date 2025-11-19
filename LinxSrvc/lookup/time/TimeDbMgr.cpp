#include "TimeDbMgr.h"
#include "logging.h"

int TimeDbMgr::connect(const std::string& db)
{
    // connect to the database
    printf("Connecting to database: %s.\n", db.c_str());
    m_connected = true; // Simulate a successful connection
    return 0;
}

bool TimeDbMgr::connected() const
{
    // check if connected to the database
    return false;
}

void TimeDbMgr::disconnect()
{
    // disconnect from the database
    printf("Disconnecting database\n");
}

void TimeDbMgr::create()
{
    char sql[SQL_LEN];
    sprintf(sql, "create table %s(id integer primary key autoincrement, %s integer, %s integer, %s integer, %s integer)",
        TABLE_SEEK_TIME, FIELD_TIMESTAMP, FIELD_OFFSET, FIELD_SIZE, FIELD_FILEID);
    PR_SQL(sql);
    sprintf(sql, "create index if not exists id on %s(%s)", TABLE_SEEK_TIME, FIELD_TIMESTAMP);
    PR_SQL(sql);
    sprintf(sql, "create table %s(id integer primary key autoincrement, %s char(128), %s integer, %s integer, %s integer, %s integer, %s char(16))",
        TABLE_FILE_MAPPING, FIELD_FILENAME,
        FIELD_FIRST_OFFSET, FIELD_LAST_OFFSET,
        FIELD_FIRST_TIME, FIELD_LAST_TIME,
        FIELD_FILESIGN);
    PR_SQL(sql);
}

int TimeDbMgr::queryTimeOffset(SelectValue seek, std::vector<SeekTimeValue>& seekOffsets, const std::string& file, int fileid)
{
    m_filename = file;
    if (!m_connected) {
        LOG_ERR("Database NOT connected!\n");
        return -1;
    }
    int fileid1 = 0; // This should be replaced with actual logic to get fileid based on filename
    getFileIdbyName(m_filename, fileid1);
    if (fileid != 0)
        fileid1 = fileid;
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "select %s,%s,%s,%s from %s where %s>=%lu and %s<=%lu and %s=%d ",
        FIELD_TIMESTAMP,
        FIELD_OFFSET,
        FIELD_SIZE,
        FIELD_FILEID,
        TABLE_SEEK_TIME,
        FIELD_TIMESTAMP,
        seek.first,
        FIELD_TIMESTAMP,
        seek.last,
        FIELD_FILEID,
        fileid1);
    PR_SQL(sql);

    return 0;
}

int TimeDbMgr::queryTimeDetail(FileTimeDetails& detail)
{
    std::string ssql = "select " FIELD_FIRST_OFFSET "," FIELD_LAST_OFFSET "," FIELD_FIRST_TIME "," FIELD_LAST_TIME
        " from " TABLE_FILE_MAPPING
        " limit 1;";
    PR_SQL(ssql.c_str());
    detail.offset = { 0, 0 };
    detail.time = { .first = 0x1, .last = 0x100000000 };
    return 0;
}

void TimeDbMgr::setDetailByFileName(const std::string& fileName, const FileTimeDetails& detail)
{
    std::string ssql = "UPDATE " TABLE_FILE_MAPPING " SET " FIELD_FIRST_OFFSET "=" + std::to_string(detail.offset.first) +
        ", " FIELD_LAST_OFFSET "=" + std::to_string(detail.offset.last) +
        ", " FIELD_FIRST_TIME "=" + std::to_string(detail.time.first) +
        ", " FIELD_LAST_TIME "=" + std::to_string(detail.time.last);
    PR_SQL(ssql.c_str());
}

void TimeDbMgr::insertContentNoDuplex(SeekTimeContent* content)
{
    int fileid = 0;
    std::string ssql = "";
    if (getFileIdbyName(m_filename, fileid) < 0) {
        printf("File ID is 0, cannot insert content!\n");
        ssql = "INSERT INTO " TABLE_FILE_MAPPING " (" FIELD_FILENAME "," FIELD_FIRST_OFFSET "," FIELD_LAST_OFFSET "," FIELD_FIRST_TIME "," FIELD_LAST_TIME
            ") SELECT '" +
            m_filename + "',0,0,-1,-1 WHERE NOT EXISTS (SELECT 1 FROM " TABLE_FILE_MAPPING " WHERE " FIELD_FILENAME " = '" +
            m_filename + "');";
        PR_SQL(ssql.c_str());
    }
    ssql = "";
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "insert into %s(%s,%s,%s,%s)",
        TABLE_SEEK_TIME,
        FIELD_TIMESTAMP,
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
    PR_SQL(ssql.c_str());
}

int TimeDbMgr::getFileIdbyName(const std::string& filename, int& fileid)
{
    // Assuming fileid is obtained from the filename
    if (m_filename.empty()) {
        LOG_ERR("Filename is NULL!\n");
        return -1;
    }
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "select id from %s where %s='%s' ",
        TABLE_FILE_MAPPING,
        FIELD_FILENAME,
        filename.c_str());
    PR_SQL(sql);
    fileid = 0;
    return 0;
}
