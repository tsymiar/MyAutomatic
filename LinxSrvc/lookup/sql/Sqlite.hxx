#pragma once
#include <string>
#ifdef USE_SQLITE3
#include <sqlite3.h>
#include <vector>
#include <cstring>
#include <cstdlib>
#endif

static const char* null = "NULL";

class Sqlite {
public:
    Sqlite(const std::string& file)
    {
        file_ = file;
    }
    ~Sqlite()
    {
#ifdef USE_SQLITE3
        db_ = nullptr;
    }
    bool open()
    {
        if (db_) return true;
        // sqlite3_open(":memory:", &db_);
        int rc = sqlite3_open(file_.c_str(), &db_);
        return (rc == SQLITE_OK);
    }
    void close()
    {
        if (db_) {
            sqlite3_close(db_);
        }
    }
    int execute(const char* sql)
    {
        if (!db_) return -1;
        char* err = nullptr;
        int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
        if (err) {
            sqlite3_free(err);
        }
        return rc;
    }
    // Returns a NULL-terminated char** containing the text values of the first row.
    // Each string is strdup'd; caller is responsible for freeing the array and each string.
    // If no rows, returns (char**)&null to match previous behavior.
    char** query(const char* sql)
    {
        if (!db_) return (char**)&null;
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return (char**)&null;
        }
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return (char**)&null;
        }
        int cols = sqlite3_column_count(stmt);
        std::vector<char*> vals;
        for (int i = 0; i < cols; ++i) {
            const unsigned char* text = sqlite3_column_text(stmt, i);
            if (text) {
                vals.push_back(strdup(reinterpret_cast<const char*>(text)));
            } else {
                vals.push_back(strdup(null));
            }
        }
        sqlite3_finalize(stmt);
        // allocate null-terminated array
        char** out = (char**)malloc((vals.size() + 1) * sizeof(char*));
        for (size_t i = 0; i < vals.size(); ++i) out[i] = vals[i];
        out[vals.size()] = nullptr;
        return out;
    }
    void free(char** pResult)
    {
        if (pResult == (char**)&null) {
            return;
        }
        for (size_t i = 0; pResult[i] != nullptr; ++i) {
            free(&pResult[i]);
        }
        free(pResult);
    }
private:
    sqlite3* db_ = nullptr;
#else
    }
    bool open() { return true; }
    void close() { }
    int execute(const char* sql) { return 0; }
    char** query(const char* sql)
    {
        return (char**)&null;
    }
    void free(char** x) { }
#endif
private:
    std::string file_;
};
