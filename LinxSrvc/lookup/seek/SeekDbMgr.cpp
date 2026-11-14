#include "SeekDbMgr.h"
#include "common.h"
#include "logging.h"

using namespace std;

int DBbase::connectDB(const string& dbName)
{
    if (m_db != nullptr) {
        LOG_WRN("database[%s] is already connected.", dbName.c_str());
        return 0;
    }
    m_db = new Sqlite(dbName);
    if (!m_db->open()) {
        LOG_ERR("sqlite3_open database(%s) error!", dbName.c_str());
        delete m_db;
        m_db = nullptr;
        return -1;
    }
    return 0;
}

bool DBbase::isConnected()
{
    return (m_db != nullptr) ? true : false;
}

int DBbase::queryTable(const std::string& sql, char*** pppResult, int& row, int& column)
{
    if (m_db == nullptr) {
        LOG_ERR("database is not connected!");
        row = 0;
        column = 0;
        return -1;
    }
    *pppResult = m_db->query(sql.c_str(), row, column);
    return (*pppResult != nullptr) ? 0 : (row > 0 ? 0 : -1);
}

void DBbase::freeTable(char** ppResult)
{
    if (ppResult == nullptr) {
        return;
    }
    if (m_db != nullptr) {
        m_db->free(ppResult);
    } else {
        LOG_WRN("attempt to free table result but database handle is null");
    }
}

int DBbase::executeSql(const std::string& sql)
{
    if (m_db == nullptr) {
        LOG_ERR("database is not connected!");
        return -1;
    }
    int rc = m_db->execute(sql.c_str());
    if (rc != 0) {
        LOG_ERR("executeSql(sql=%s) failed(%d)!", sql.c_str(), rc);
        return rc;
    }
    return 0;
}

bool DBbase::isTableExist(const std::string& tableName)
{
#ifdef USE_SQLITE3
    // 使用 sqlite3_mprintf 的 %q 格式化防止 SQL 注入
    char* safeName = sqlite3_mprintf("%q", tableName.c_str());
    if (!safeName) return false;

    char a_sql[256] = { 0 };
    snprintf(a_sql, sizeof(a_sql),
        "select count(*) as cnt from sqlite_master where type='table' and name='%s'",
        safeName);
    sqlite3_free(safeName);
    string str_sql = a_sql;
#else
    // 非 SQLITE3 模式：直接拼接（仅用于调试输出）
    char a_sql[256] = { 0 };
    snprintf(a_sql, sizeof(a_sql),
        "select count(*) as cnt from sqlite_master where type='table' and name='%s'",
        tableName.c_str());
    string str_sql = a_sql;
#endif

    char** ppResult = nullptr;
    int row = 0;
    int column = 0;
    int i_ret = queryTable(str_sql, &ppResult, row, column);
    if (i_ret != 0) {
        return false;
    }
    if (row < 1) {
        freeTable(ppResult);
        return false;
    }
    bool exists = (ppResult[0] != NULL && atoi(ppResult[0]) == 1);
    freeTable(ppResult);
    return exists;
}

void DBbase::disconnectDB()
{
    if (m_db != nullptr) {

        m_db->close();
        delete m_db;
        m_db = nullptr;
    }
    return;
}

SeekDbMgr::SeekDbMgr()
{}

SeekDbMgr::~SeekDbMgr()
{}

bool SeekDbMgr::isTableExist()
{
    char a_sql[256] = { 0 };
    // TABLE_SEEK_TIME 是宏常量，无需参数化；但仍用 snprintf 防溢出
    snprintf(a_sql, sizeof(a_sql),
        "select count(*) as cnt from sqlite_master where type='table' and name='%s'",
        TABLE_SEEK_TIME);
    string sqlStatement = a_sql;

    char** ppResult = nullptr;
    int row = 0;
    int column = 0;
    int i_ret = queryTable(sqlStatement, &ppResult, row, column);
    if (i_ret != 0) {
        return false;
    }
    if (row < 1) {
        freeTable(ppResult);
        return false;
    }
    bool exists = (ppResult[0] != NULL && atoi(ppResult[0]) == 1);
    freeTable(ppResult);
    return exists;
}

int SeekDbMgr::createTable()
{
    if (isTableExist()) {
        return clearTable();
    }
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN, "create table %s(id integer primary key autoincrement, %s integer, %s large integer, %s integer, %s integer, %s integer)",
        TABLE_SEEK_TIME, FIELD_FILEID, FIELD_TIMESTAMP, FIELD_DURATION, FIELD_OFFSET, FIELD_SIZE);
    string sqlStatement = sql;

    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = executeSql(sqlStatement);
    if (ret != 0) {
        LOG_ERR("create table %s failed(%d)!.", TABLE_SEEK_TIME, ret);
        return ret;
    }

    char tmpIndex[SQL_LEN] = { 0 };
    snprintf(tmpIndex, SQL_LEN, "create index if not exists idIndex on %s(%s)",
        TABLE_SEEK_TIME, FIELD_TIMESTAMP);
    string sqlCreateIndex = tmpIndex;
    printf("SQLITE:[ %s ]\n", sqlCreateIndex.c_str());
    ret = executeSql(sqlCreateIndex);
    if (ret != 0) {
        LOG_ERR("executeSql(sql=%s) error: %d!", sqlCreateIndex.c_str(), ret);
    }

    memset(sql, 0, SQL_LEN);
    snprintf(sql, SQL_LEN, "create table %s(id integer primary key autoincrement, %s char(128), %s integer, %s integer, %s large integer, %s large integer, %s char(16))",
        TABLE_FILE_MAPPING, FIELD_FILENAME,
        FIELD_FIRST_OFFSET, FIELD_LAST_OFFSET,
        FIELD_FIRST_TIME, FIELD_LAST_TIME,
        FIELD_FILESIGN);
    printf("SQLITE:[ %s ]\n", sql);
    ret = executeSql(std::string(sql));
    if (ret != 0) {
        LOG_ERR("create table %s failed(%d)!.", TABLE_SEEK_TIME, ret);
        return ret;
    }

    return 0;
}

int SeekDbMgr::clearTable()
{
    std::lock_guard<std::mutex> lock(m_lock);
    const char* tables[] = { TABLE_SEEK_TIME, TABLE_FILE_MAPPING };
    for (const char* table : tables) {
        if (!DBbase::isTableExist(table)) {
            continue;
        }
        char sql[SQL_LEN] = { 0 };
        snprintf(sql, SQL_LEN, "delete from %s", table);
        string sqlStatement = sql;
        printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
        int ret = executeSql(sqlStatement);
        if (ret != 0) {
            LOG_ERR("clear %s failed(sql=%s) error: %d!", table, sqlStatement.c_str(), ret);
            return ret;
        }
    }
    return 0;
}

int SeekDbMgr::insertFileIdbyName(FileFrameData* frame)
{
    int ret = queryFileIdbyName(frame->fileName, frame->id);
    if (ret == 0) {
        LOG_WRN("file already exist in %s, update detail values.", TABLE_FILE_MAPPING);
        char csql[SQL_LEN] = { 0 };
        snprintf(csql, SQL_LEN, "update %s set %s=%ld,%s=%ld,%s=%lu,%s=%lu where %s=%u",
            TABLE_FILE_MAPPING,
            FIELD_FIRST_OFFSET, frame->offset.head,
            FIELD_LAST_OFFSET, frame->offset.tail,
            FIELD_FIRST_TIME, frame->ftime.first,
            FIELD_LAST_TIME, frame->ftime.last,
            TABLE_INDEX, frame->id);
        std::string sql = csql;
        printf("SQLITE:[ %s ]\n", sql.c_str());
        ret = executeSql(sql);
        if (ret != 0) {
            LOG_ERR("executeSql(sql=%s) error: %d!", sql.c_str(), ret);
            return ret;
        }
        return 0;
    }

#ifdef USE_SQLITE3
    // 对文件名做安全转义，防 SQL 注入
    char* safeName = sqlite3_mprintf("%q", frame->fileName);
    char* safeSign = sqlite3_mprintf("%q", frame->fileSign);
    if (!safeName || !safeSign) {
        if (safeName) sqlite3_free(safeName);
        if (safeSign) sqlite3_free(safeSign);
        return -1;
    }
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN,
        "insert into %s(%s,%s,%s,%s,%s,%s) VALUES ('%s',%ld,%ld,%lu,%lu,'%s')",
        TABLE_FILE_MAPPING,
        FIELD_FILENAME, FIELD_FIRST_OFFSET, FIELD_LAST_OFFSET,
        FIELD_FIRST_TIME, FIELD_LAST_TIME, FIELD_FILESIGN,
        safeName,
        frame->offset.head, frame->offset.tail,
        frame->ftime.first, frame->ftime.last,
        safeSign);
    sqlite3_free(safeName);
    sqlite3_free(safeSign);
#else
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN,
        "insert into %s(%s,%s,%s,%s,%s,%s) VALUES ('%s',%ld,%ld,%lu,%lu,'%s')",
        TABLE_FILE_MAPPING,
        FIELD_FILENAME, FIELD_FIRST_OFFSET, FIELD_LAST_OFFSET,
        FIELD_FIRST_TIME, FIELD_LAST_TIME, FIELD_FILESIGN,
        frame->fileName,
        frame->offset.head, frame->offset.tail,
        frame->ftime.first, frame->ftime.last,
        frame->fileSign);
#endif

    string sqlStatement = sql;
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    ret = executeSql(sqlStatement);
    if (ret != 0) {
        LOG_ERR("executeSql(sql=%s) error: %d!", sqlStatement.c_str(), ret);
        return ret;
    }
    ret = queryFileIdbyName(frame->fileName, frame->id);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int SeekDbMgr::addIndexNoDuplex(void* pIndex)
{
    std::lock_guard<std::mutex> lock(m_lock);
    SeekTimeContent* content = (SeekTimeContent*)pIndex;

    int ret = queryFileIdbyName(content->fileName, content->fileid);
    if (ret != 0) {
        LOG_WRN("file '%s' not exist in " TABLE_FILE_MAPPING, content->fileName);
        return ret;
    }
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN,
        "insert into %s(%s,%s,%s,%s,%s) values (%u,%lld,%d,%lld,%lld)",
        TABLE_SEEK_TIME,
        FIELD_FILEID, FIELD_TIMESTAMP, FIELD_DURATION, FIELD_OFFSET, FIELD_SIZE,
        content->fileid, content->value.timestamp, content->duration,
        content->value.offset, content->value.size);

    string sqlStatement = sql;

    ret = executeSql(sqlStatement);
    if (ret != 0) {
        LOG_ERR("executeSql(sql=%s) error: %d!", sqlStatement.c_str(), ret);
        return ret;
    }
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());

    return 0;
}

int SeekDbMgr::getTargetFragmentByTime(int64_t time, int32_t duration,
    SeekTimeContent* content)
{
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN, "select %s,%s,%s from %s where %s=%u and %s=%ld and %s=%d limit 1",
        FIELD_TIMESTAMP, FIELD_OFFSET, FIELD_SIZE,
        TABLE_SEEK_TIME,
        FIELD_FILEID, content->fileid,
        FIELD_TIMESTAMP, time,
        FIELD_DURATION, duration);
    string sqlStatement = sql;

    int row = 0;
    int column = 0;
    char** ppResult = nullptr;
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = queryTable(sqlStatement, &ppResult, row, column);
    if (ret != 0) {
        LOG_ERR("queryTable(sql=%s) error: %d!", sqlStatement.c_str(), ret);
        return ret;
    }

    if (row <= 0) {
        LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0!",
            sqlStatement.c_str(), row);
        freeTable(ppResult);
        return -1;
    }

    if (column < 3) {
        LOG_ERR("queryTable(sql=%s) error: column[%d] is less than expected!",
            sqlStatement.c_str(), column);
        freeTable(ppResult);
        return -2;
    }
    content->value.offset = atoll(ppResult[column - 2]);
    content->value.size += atoll(ppResult[column - 1]);
    content->duration = duration;
    content->value.timestamp = time;
    content->found = true;

    freeTable(ppResult);
    return 0;
}

int SeekDbMgr::queryFileIdbyName(const std::string& filename, uint32_t& fileid)
{
#ifdef USE_SQLITE3
    char* safeName = sqlite3_mprintf("%q", filename.c_str());
    if (!safeName) return -1;
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN, "select id from %s where %s='%s'",
        TABLE_FILE_MAPPING, FIELD_FILENAME, safeName);
    sqlite3_free(safeName);
#else
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN, "select id from %s where %s='%s'",
        TABLE_FILE_MAPPING, FIELD_FILENAME, filename.c_str());
#endif
    string sqlStatement = sql;
    int row = 0;
    int column = 0;
    char** ppResult = nullptr;
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = queryTable(sqlStatement, &ppResult, row, column);
    if (ret != 0) {
        LOG_ERR("queryTable(sql=%s) error: %d!", sqlStatement.c_str(), ret);
        return ret;
    }
    if (row <= 0) {
        LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0!",
            sqlStatement.c_str(), row);
        freeTable(ppResult);
        return -1;
    }
    fileid = atol(ppResult[0]);

    freeTable(ppResult);
    return 0;
}

int SeekDbMgr::getTimeOffsetByFileName(const std::string& filename, const SelectValue timeValue,
    FileFrameData& firstframe, FileFrameData& lastframe)
{
    uint32_t fileid = 0;
    int ret = queryFileIdbyName(filename, fileid);
    if (ret != 0) {
        LOG_ERR("error find filename [%s] in dataBase!", filename.c_str());
        return ret;
    }

    struct Boundary {
        int64_t timestamp;
        int64_t offset;
    };

    /* Clamp to the file's first/last frame when time is out of range. */
    auto queryNearest = [this, fileid](long long time, bool atOrBefore,
                            Boundary& out) -> int {
        const char* cmp = atOrBefore ? "<=" : ">=";
        const char* order = atOrBefore ? "desc" : "asc";
        const char* extreme = atOrBefore ? "min" : "max";
        char sql[SQL_LEN] = { 0 };
        snprintf(sql, SQL_LEN,
            "select %s,%s,%s from %s where %s=%u and %s<0 and (%s%s%lld or %s=(select %s(%s) from %s where %s=%u and %s<0)) order by %s %s limit 1",
            FIELD_TIMESTAMP, FIELD_OFFSET, FIELD_SIZE, TABLE_SEEK_TIME,
            FIELD_FILEID, fileid, FIELD_DURATION,
            FIELD_TIMESTAMP, cmp, time,
            FIELD_TIMESTAMP, extreme, FIELD_TIMESTAMP, TABLE_SEEK_TIME,
            FIELD_FILEID, fileid, FIELD_DURATION,
            FIELD_TIMESTAMP, order);
        string sqlStatement = sql;
        printf("SQLITE:[ %s ]\n", sqlStatement.c_str());

        char** ppResult = nullptr;
        int row = 0;
        int column = 0;
        int rc = queryTable(sqlStatement, &ppResult, row, column);
        if (rc != 0 || row < 1 || column < 2) {
            LOG_WRN("queryTable(sql=%s) got row[%d] column[%d] ret=%d!",
                sqlStatement.c_str(), row, column, rc);
            freeTable(ppResult);
            return -1;
        }
        out.timestamp = atoll(ppResult[0]);
        out.offset = atoll(ppResult[1]);
        freeTable(ppResult);
        return 0;
    };

    const long long first = static_cast<long long>(timeValue.first);
    const long long last = static_cast<long long>(timeValue.last);
    Boundary leftHead{}, leftTail{}, rightHead{}, rightTail{};
    if (queryNearest(first, true, leftHead) != 0) {
        return -1;
    }
    if (queryNearest(first, false, leftTail) != 0) {
        return -2;
    }
    if (queryNearest(last, true, rightHead) != 0) {
        return -3;
    }
    if (queryNearest(last, false, rightTail) != 0) {
        return -4;
    }

    firstframe.ftime.first = leftHead.timestamp;
    firstframe.offset.head = leftHead.offset;
    firstframe.ftime.last = leftTail.timestamp;
    firstframe.offset.tail = leftTail.offset;
    firstframe.id = fileid;

    lastframe.ftime.first = rightHead.timestamp;
    lastframe.offset.head = rightHead.offset;
    lastframe.ftime.last = rightTail.timestamp;
    lastframe.offset.tail = rightTail.offset;
    lastframe.id = fileid;

    return 0;
}

int SeekDbMgr::queryFileTime(const string& filename, SelectTime& time)
{
#ifdef USE_SQLITE3
    char* safeName = sqlite3_mprintf("%q", filename.c_str());
    if (!safeName) return -1;
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN, "select %s,%s from %s where %s='%s'",
        FIELD_FIRST_TIME, FIELD_LAST_TIME,
        TABLE_FILE_MAPPING, FIELD_FILENAME, safeName);
    sqlite3_free(safeName);
#else
    char sql[SQL_LEN] = { 0 };
    snprintf(sql, SQL_LEN, "select %s,%s from %s where %s='%s'",
        FIELD_FIRST_TIME, FIELD_LAST_TIME,
        TABLE_FILE_MAPPING, FIELD_FILENAME, filename.c_str());
#endif
    string sqlStatement = sql;
    int row = 0;
    int column = 0;
    char** ppResult = nullptr;
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = queryTable(sqlStatement, &ppResult, row, column);
    if (ret != 0) {
        LOG_ERR("queryTable(sql=%s) error: %d!", sqlStatement.c_str(), ret);
        return ret;
    }
    if (row <= 0) {
        LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0!",
            sqlStatement.c_str(), row);
        freeTable(ppResult);
        return -1;
    }
    time.first = atoll(ppResult[0]);
    time.last = atoll(ppResult[1]);

    freeTable(ppResult);
    return 0;
}
