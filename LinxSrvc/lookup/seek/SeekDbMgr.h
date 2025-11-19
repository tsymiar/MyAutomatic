#pragma once

#include <queue>
#include <mutex>
#include "SeekComm.h"
#include "sql/DBComm.h"

class DBbase {
public:
	int connectDB(const std::string& dbName);
	bool isConnected();
	int queryTable(const std::string& sql, char*** pResult, int& row, int& column);
	void freeTable(char** pResult);
	int executeSql(const std::string& sql);
	bool isTableExist(const std::string& tableName);
	void disconnectDB();
private:
	// sqlite3* m_db;
	Sqlite* m_db = nullptr;
};

class SeekDbMgr : public DBbase {
public:
	SeekDbMgr();
	virtual ~SeekDbMgr();
	virtual bool isTableExist();

	virtual int createTable();
	virtual int addIndexNoDuplex(void* pIndex);

	virtual int getTargetFragmentByTime(int64_t time, int32_t duration, SeekTimeContent* content);

	virtual int getTimeOffsetByFileName(const std::string& filename, const SelectValue timeValue,
		FileDataFrame& firstframe, FileDataFrame& lastframe);

	virtual int insertFileIdbyName(FileDataFrame* frame);

	virtual int queryFileIdbyName(const std::string& filename, uint32_t& fileid);

	virtual int queryFileTime(const std::string& filename, SelectTime& time);

private:
	std::mutex m_lock;
};
