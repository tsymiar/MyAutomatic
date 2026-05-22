#pragma once
#include <string>
#ifdef USE_SQLITE3
#include <sqlite3.h>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
#endif

static const char* null = "NULL";

class Sqlite {
public:
    Sqlite(const std::string& file) : file_(file) {}
    ~Sqlite() { close(); }
    bool open();
    void close();
    int execute(const char* sql);
    char** query(const char* sql, int& row, int& column);
    char** queryParam(const char* sql, int& row, int& column, ...);
    void free(char** pResult);

private:
#ifdef USE_SQLITE3
    sqlite3* db_ = nullptr;
#endif
    std::string file_;
};

// ============================================================================
// USE_SQLITE3 宏定义时：真实 sqlite3 实现
// ============================================================================
#ifdef USE_SQLITE3

inline bool Sqlite::open()
{
    if (db_) return true;
    int rc = sqlite3_open(file_.c_str(), &db_);
    return (rc == SQLITE_OK);
}

inline void Sqlite::close()
{
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

inline int Sqlite::execute(const char* sql)
{
    if (!db_) return -1;
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (err) {
        sqlite3_free(err);
    }
    return rc;
}

/**
 * 执行查询，通过输出参数返回行数和列数。
 * @param sql    SQL 语句
 * @param row    输出：结果行数
 * @param column 输出：结果列数
 * @return       堆分配的 NULL 结尾 char** 数组（按行×列顺序存储）。
 *               无结果时返回 nullptr。
 */
inline char** Sqlite::query(const char* sql, int& row, int& column)
{
    row = 0;
    column = 0;
    if (!db_) return nullptr;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }

    column = sqlite3_column_count(stmt);

    // 收集所有行
    std::vector<char*> allVals;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (int i = 0; i < column; ++i) {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            if (text) {
                allVals.push_back(strdup(reinterpret_cast<const char*>(text)));
            } else {
                allVals.push_back(strdup(null));
            }
        }
        ++row;
    }
    sqlite3_finalize(stmt);

    if (row == 0) {
        return nullptr;
    }

    // 分配 row×col + 1 的 NULL 结尾数组（按行存储）
    size_t total = static_cast<size_t>(row) * column;
    char** out = (char**)malloc((total + 1) * sizeof(char*));
    for (size_t i = 0; i < total; ++i) {
        out[i] = allVals[i];
    }
    out[total] = nullptr;
    return out;
}

/**
 * 安全的参数化查询（使用 sqlite3_bind_* 防 SQL 注入）。
 * 调用方式：
 *    int row, col;
 *    char** result = db.queryParam(
 *        "select id from t where name=?1 and value>?2",
 *        row, col,
 *        "s", "tablename",    // TEXT 参数
 *        "d", (int64_t)42,    // INT64 参数
 *        nullptr              // 终止标记（必须）
 *    );
 * 格式符: "s"=TEXT, "d"=INT64, "i"=INT32
 * @note 结尾必须传 nullptr 作为参数终止标记
 */
inline char** Sqlite::queryParam(const char* sql, int& row, int& column, ...)
{
    row = 0;
    column = 0;
    if (!db_) return nullptr;

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return nullptr;
    }

    va_list args;
    va_start(args, column);
    int idx = 1;
    while (true) {
        const char* fmt = va_arg(args, const char*);
        if (!fmt) break;
        switch (fmt[0]) {
        case 's': {
            const char* val = va_arg(args, const char*);
            sqlite3_bind_text(stmt, idx++, val, -1, SQLITE_TRANSIENT);
            break;
        }
        case 'd': {
            int64_t val = va_arg(args, int64_t);
            sqlite3_bind_int64(stmt, idx++, val);
            break;
        }
        case 'i': {
            int val = va_arg(args, int);
            sqlite3_bind_int(stmt, idx++, val);
            break;
        }
        default:
            break;
        }
    }
    va_end(args);

    column = sqlite3_column_count(stmt);
    std::vector<char*> allVals;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        for (int i = 0; i < column; ++i) {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            allVals.push_back(text
                ? strdup(reinterpret_cast<const char*>(text))
                : strdup(null));
        }
        ++row;
    }
    sqlite3_finalize(stmt);

    if (row == 0) return nullptr;
    size_t total = static_cast<size_t>(row) * column;
    char** out = (char**)malloc((total + 1) * sizeof(char*));
    for (size_t i = 0; i < total; ++i) out[i] = allVals[i];
    out[total] = nullptr;
    return out;
}

inline void Sqlite::free(char** pResult)
{
    if (!pResult) return;
    for (size_t i = 0; pResult[i] != nullptr; ++i) {
        ::free(pResult[i]);
    }
    ::free(pResult);
}

// ============================================================================
// 未定义 USE_SQLITE3 时：空实现桩（stub）
// ============================================================================
#else

inline bool Sqlite::open() { return true; }
inline void Sqlite::close() { }
inline int Sqlite::execute(const char* sql) { (void)sql; return 0; }
inline char** Sqlite::query(const char* sql, int& row, int& column)
{
    (void)sql; row = 0; column = 0;
    return nullptr;
}
inline char** Sqlite::queryParam(const char* sql, int& row, int& column, ...)
{
    (void)sql; row = 0; column = 0;
    return nullptr;
}
inline void Sqlite::free(char** x) { (void)x; }

#endif
