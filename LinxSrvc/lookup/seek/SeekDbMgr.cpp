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

int DBbase::queryTable(const std::string& sql, char*** pResult, int& row, int& column)
{
    if (m_db == nullptr) {
        LOG_ERR("database is not connected!");
        return -1;
    }
    *pResult = m_db->query(sql.c_str());
    return 0;
}

void DBbase::freeTable(char** pResult)
{
    if (pResult == nullptr) {
        return;
    }
    if (m_db != nullptr) {
        m_db->free(pResult);
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
    m_db->execute(sql.c_str());
    return 0;
}

bool DBbase::isTableExist(const std::string& tableName)
{
    char a_sql[256] = { 0 };
    sprintf(a_sql, "select count(*) as cnt from sqlite_master where type='table' and name='%s'",
        tableName.c_str());
    string str_sql = a_sql;

    char** pResult = nullptr;
    int row = 0;
    int column = 0;
    int i_ret = queryTable(str_sql, &pResult, row, column);
    if (i_ret != 0) {
        return false;
    }
    if (row != 1) {
        return false;
    }
    if (atoi(pResult[column]) == 1) {
        return true;
    }
    return false;
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
{ }

SeekDbMgr::~SeekDbMgr()
{ }

bool SeekDbMgr::isTableExist()
{
    char a_sql[256] = { 0 };
    sprintf(a_sql, "select count(*) as cnt from sqlite_master where type='table' and name='%s'",
        TABLE_SEEK_TIME);
    string sqlStatement = a_sql;

    char** pResult = nullptr;
    int row = 0;
    int column = 0;
    int i_ret = queryTable(sqlStatement, &pResult, row, column);
    if (i_ret != 0) {
        return false;
    }
    if (row != 1) {
        return false;
    }
    if (atoi(pResult[column]) == 1) {
        return true;
    }
    return false;
}

int SeekDbMgr::createTable()
{
    if (isTableExist()) {
        return 0;
    }
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "create table %s(id integer primary key autoincrement, %s integer, %s large integer, %s integer, %s integer, %s integer)",
        TABLE_SEEK_TIME, FIELD_FILEID, FIELD_TIMESTAMP, FIELD_DURATION, FIELD_OFFSET, FIELD_SIZE);
    string sqlStatement = sql;

    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = executeSql(sqlStatement);
    if (ret != 0) {
        LOG_ERR("create table %s failed!(0x%08X).", TABLE_SEEK_TIME, ret);
        return ret;
    }

    char tmpIndex[SQL_LEN] = { 0 };
    sprintf(tmpIndex, "create index if not exists idIndex on %s(%s)",
        TABLE_SEEK_TIME, FIELD_TIMESTAMP);
    string sqlCreateIndex = tmpIndex;
    printf("SQLITE:[ %s ]\n", sqlCreateIndex.c_str());
    ret = executeSql(sqlCreateIndex);
    if (ret != 0) {
        LOG_ERR("executeSql(sql=%s) error: 0x%08X!", sqlCreateIndex.c_str(), ret);
    }

    memset(sql, SQL_LEN, 0);
    sprintf(sql, "create table %s(id integer primary key autoincrement, %s char(128), %s integer, %s integer, %s large integer, %s large integer, %s char(16))",
        TABLE_FILE_MAPPING, FIELD_FILENAME,
        FIELD_FIRST_OFFSET, FIELD_LAST_OFFSET,
        FIELD_FIRST_TIME, FIELD_LAST_OFFSET,
        FIELD_FILESIGN);
    printf("SQLITE:[ %s ]\n", sql);
    ret = executeSql(std::string(sql));
    if (ret != 0) {
        LOG_ERR("create table %s failed!(0x%08X).", TABLE_SEEK_TIME, ret);
        return ret;
    }

    return 0;
}

int SeekDbMgr::insertFileIdbyName(FileDataFrame* frame)
{
    int ret = queryFileIdbyName(frame->fileName, frame->id);
    if (ret == 0) {
        LOG_WRN("file already exist in %s, update values. ", TABLE_FILE_MAPPING);
        char csql[SQL_LEN] = { 0 };
        sprintf(csql, "update %s set %s=%lld,%s=%lld,%s=%lld,%s=%lld where %s=%d",
            TABLE_FILE_MAPPING,
            FIELD_FIRST_OFFSET,
            frame->offset.head,
            FIELD_LAST_OFFSET,
            frame->offset.tail,
            FIELD_FIRST_TIME,
            frame->ftime.first,
            FIELD_LAST_OFFSET,
            frame->ftime.last,
            TABLE_INDEX,
            frame->id);
        std::string sql = csql;
        printf("SQLITE:[ %s ]\n", sql.c_str());
        ret = executeSql(sql);
        if (ret != 0) {
            LOG_ERR("executeSql(sql=%s) error: 0x%08X!", sql.c_str(), ret);
            return ret;
        }
        return 0;
    }

    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "insert into %s(%s,%s,%s,%s,%s,%s)",
        TABLE_FILE_MAPPING,
        FIELD_FILENAME,
        FIELD_FIRST_OFFSET,
        FIELD_LAST_OFFSET,
        FIELD_FIRST_TIME,
        FIELD_LAST_OFFSET,
        FIELD_FILESIGN);

    string sqlStatement = "";
    sqlStatement += sql;

    memset(sql, SQL_LEN, 0);
    sprintf(sql, "VALUES ('%s',%lld,%lld,%lld,%lld,'%s')",
        frame->fileName,
        frame->offset.head,
        frame->offset.tail,
        frame->ftime.first,
        frame->ftime.last,
        frame->fileSign);

    sqlStatement += sql;

    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    ret = executeSql(sqlStatement);
    if (ret != 0) {
        LOG_ERR("executeSql(sql=%s) error: 0x%08X!", sqlStatement.c_str(), ret);
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
    string sqlStatement = "";
    char sql[SQL_LEN] = { 0 };

    int ret = queryFileIdbyName(content->fileName, content->fileid);
    if (ret != 0) {
        LOG_WRN("file not exist in TblFileIdMapping");
        return ret;
    }
    sprintf(sql, "insert into %s(%s,%s,%s,%s,%s)",
        TABLE_SEEK_TIME,
        FIELD_FILEID,
        FIELD_TIMESTAMP,
        FIELD_DURATION,
        FIELD_OFFSET,
        FIELD_SIZE);
    sqlStatement += sql;

    sprintf(sql, "values (%u,%lld,%d,%lld,%lld)",
        content->fileid,
        content->value.timestamp,
        content->duration,
        content->value.offset,
        content->value.size);
    sqlStatement += sql;

    ret = executeSql(sqlStatement);
    if (ret != 0) {
        LOG_ERR("executeSql(sql=%s) error: 0x%08X!", sqlStatement.c_str(), ret);
        return ret;
    }
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());

    return 0;
}

int SeekDbMgr::getTargetFragmentByTime(int64_t time, int32_t duration,
    SeekTimeContent* content)
{
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "select %s,%s,%s from %s where %s=%u and %s=%lld and %s=%u limit 1",
        FIELD_TIMESTAMP,
        FIELD_OFFSET,
        FIELD_SIZE,
        TABLE_SEEK_TIME,
        FIELD_FILEID,
        content->fileid,
        FIELD_TIMESTAMP,
        time,
        FIELD_DURATION,
        duration);
    string sqlStatement = sql;

    int row = 0;
    int column = 0;
    char** ppResult;
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = queryTable(sqlStatement, &ppResult, row, column);
    if (ret != 0) {
        LOG_ERR("queryTable(sql=%s) error: 0x%08X!", sqlStatement.c_str(), ret);
        return ret;
    }

    if (row <= 0) {
        LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0 ",
            sqlStatement.c_str(), row);
        freeTable(ppResult);
        return -1;
    }

    int index = column;
    content->value.offset = atoll(ppResult[index + 1]);
    content->value.size += atoll(ppResult[index + 2]);
    content->duration = duration;
    content->value.timestamp = time;
    content->found = true;

    freeTable(ppResult);
    return 0;
}

int SeekDbMgr::queryFileIdbyName(const std::string& filename, uint32_t& fileid)
{
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "select id from %s where %s='%s' ",
        TABLE_FILE_MAPPING,
        FIELD_FILENAME,
        filename.c_str());
    string sqlStatement = sql;
    int row = 0;
    int column = 0;
    char** ppResult;
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = queryTable(sqlStatement, &ppResult, row, column);
    if (ret != 0) {
        LOG_ERR("queryTable(sql=%s) error: 0x%08X!", sqlStatement.c_str(), ret);
        return ret;
    }
    if (row <= 0) {
        LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0 ",
            sqlStatement.c_str(), row);
        freeTable(ppResult);
        return -1;
    }
    fileid = atol(ppResult[column]);

    freeTable(ppResult);
    return 0;
}

int SeekDbMgr::getTimeOffsetByFileName(const std::string& filename, const SelectValue timeValue,
    FileDataFrame& firstframe, FileDataFrame& lastframe)
{
    uint32_t fileid = 0;
    int ret = queryFileIdbyName(filename, fileid);
    if (ret != 0) {
        LOG_ERR("error find filename[%s] in dataBase failed!", filename.c_str());
        return ret;
    }
    char sql[SQL_LEN] = { 0 };
    int row = 0;
    int column = 0;
    char** ppResult;
    int index = 0;

    /* seek left */
    {
        sprintf(sql, "select %s,%s,%s from %s where %s=%u and %s<=%lld and %s<0 order by %s desc limit 1",
            FIELD_TIMESTAMP, FIELD_OFFSET, FIELD_SIZE, TABLE_SEEK_TIME, FIELD_FILEID, fileid, FIELD_TIMESTAMP, timeValue.first, FIELD_DURATION, FIELD_TIMESTAMP);
        string sqlStatement = sql;
        printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
        ret = queryTable(sqlStatement, &ppResult, row, column);
        if (ret != 0) {
            LOG_ERR("error queryTable[%s] fileid=%d in dataBase failed!", filename.c_str(), fileid);
            return ret;
        }

        if (row <= 0) {
            LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0 ",
                sqlStatement.c_str(), row);
            freeTable(ppResult);
            return -1;
        }
        if (row == 1) {
            index = column;
            firstframe.ftime.first = atoll(ppResult[index]);
            firstframe.offset.head = atoll(ppResult[index + 1]);
        }
        freeTable(ppResult);

        memset(&sql, 0, SQL_LEN);
        sprintf(sql, "select %s,%s,%s from %s where %s=%u and %s>=%lld and %s<0  order by %s asc limit 1",
            FIELD_TIMESTAMP, FIELD_OFFSET, FIELD_SIZE, TABLE_SEEK_TIME, FIELD_FILEID, fileid, FIELD_TIMESTAMP, timeValue.first, FIELD_DURATION, FIELD_TIMESTAMP);
        string sqlStatement1 = sql;
        printf("SQLITE:[ %s ]\n", sqlStatement1.c_str());
        ret = queryTable(sqlStatement1, &ppResult, row, column);
        if (ret != 0) {
            LOG_ERR("queryTable(sql=%s) error: 0x%08X!", sqlStatement1.c_str(), ret);
            return ret;
        }

        if (row <= 0) {
            LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0 ",
                sqlStatement1.c_str(), row);
            freeTable(ppResult);
            return -2;
        }
        if (row == 1) {
            index = column;
            firstframe.ftime.last = atoll(ppResult[index]);
            firstframe.offset.tail = atoll(ppResult[index + 1]);
        }
        firstframe.id = fileid;
        freeTable(ppResult);
    }

    /* seek right */
    {
        sprintf(sql, "select %s,%s,%s from %s where %s=%u and %s<=%lld and %s<0 order by %s desc limit 1",
            FIELD_TIMESTAMP, FIELD_OFFSET, FIELD_SIZE, TABLE_SEEK_TIME, FIELD_FILEID, fileid, FIELD_TIMESTAMP, timeValue.last, FIELD_DURATION, FIELD_TIMESTAMP);
        string sqlStatement3 = sql;

        row = 0;
        column = 0;
        printf("SQLITE:[ %s ]\n", sqlStatement3.c_str());
        ret = queryTable(sqlStatement3, &ppResult, row, column);
        if (ret != 0) {
            LOG_ERR("queryTable(sql=%s) error: 0x%08X!", sqlStatement3.c_str(), ret);
            return ret;
        }

        if (row <= 0) {
            LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0 ",
                sqlStatement3.c_str(), row);
            freeTable(ppResult);
            return -3;
        }
        if (row == 1) {
            index = column;
            lastframe.ftime.first = atoll(ppResult[index]);
            lastframe.offset.head = atoll(ppResult[index + 1]);
        }
        freeTable(ppResult);

        memset(&sql, 0, SQL_LEN);
        sprintf(sql, "select %s,%s,%s from %s where %s=%u and %s>=%lld and %s<0 order by %s asc limit 1",
            FIELD_TIMESTAMP, FIELD_OFFSET, FIELD_SIZE, TABLE_SEEK_TIME, FIELD_FILEID, fileid, FIELD_TIMESTAMP, timeValue.last, FIELD_DURATION, FIELD_TIMESTAMP);
        string sqlStatement4 = sql;

        row = 0;
        column = 0;
        printf("SQLITE:[ %s ]\n", sqlStatement4.c_str());
        ret = queryTable(sqlStatement4, &ppResult, row, column);
        if (ret != 0) {
            LOG_ERR("queryTable(sql=%s) error: 0x%08X!", sqlStatement4.c_str(), ret);
            return ret;
        }

        if (row <= 0) {
            LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0 ",
                sqlStatement4.c_str(), row);
            freeTable(ppResult);
            return -4;
        }
        if (row == 1) {
            index = column;
            lastframe.ftime.last = atoll(ppResult[index]);
            lastframe.offset.tail = atoll(ppResult[index + 1]);
        }

        lastframe.id = fileid;
        freeTable(ppResult);
    }

    return 0;
}

int SeekDbMgr::queryFileTime(const string& filename, SelectTime& time)
{
    char sql[SQL_LEN] = { 0 };
    sprintf(sql, "select %s,%s from %s where %s='%s' ",
        FIELD_FIRST_TIME,
        FIELD_LAST_OFFSET,
        TABLE_FILE_MAPPING,
        FIELD_FILENAME,
        filename.c_str());
    string sqlStatement = sql;
    int row = 0;
    int column = 0;
    char** ppResult;
    printf("SQLITE:[ %s ]\n", sqlStatement.c_str());
    int ret = queryTable(sqlStatement, &ppResult, row, column);
    if (ret != 0) {
        LOG_ERR("queryTable(sql=%s) error: 0x%08X!", sqlStatement.c_str(), ret);
        return ret;
    }
    if (row <= 0) {
        LOG_WRN("queryTable(sql=%s) error: row[%d] is not greater than 0 ",
            sqlStatement.c_str(), row);
        freeTable(ppResult);
        return -1;
    }
    time.first = atoll(ppResult[column]);
    time.last = atoll(ppResult[column + 1]);

    freeTable(ppResult);
    return 0;
}
