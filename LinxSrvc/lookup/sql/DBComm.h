#pragma once
#include <string>
#include <vector>
#include "common.h"
#include "Sqlite.hxx"

#define TABLE_FILE_MAPPING "TblFileMapping"
#define TABLE_SEEK_TIME "TblSeekTime"
#define TABLE_INDEX "id"
#define FIELD_TIMESTAMP "Timestamp"
#define FIELD_DURATION "Duration"
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
#define PR_SQL(sql) printf("sql:[%s](%s %d)\n", sql, __func__, __LINE__)
