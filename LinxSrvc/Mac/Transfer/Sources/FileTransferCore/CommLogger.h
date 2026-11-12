#ifndef FILE_TRANSFER_COMLOG_H
#define FILE_TRANSFER_COMLOG_H
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>
#include <vector>
#include <atomic>

#ifndef LOG_TAG
#define LOG_TAG "Transfer"
#endif

#pragma GCC diagnostic ignored "-Wformat"

// Log sink: TransferBridge.cpp points it at the Swift UI; NULL (default) keeps logs on stdout only.
typedef void (*LogOutputFunc)(const char* msg);
extern std::atomic<LogOutputFunc> g_logOutputFunc;

#ifdef NOLOG
#define LOG_INF(fmt, ...)  ((void)0)
#define LOG_WRN(fmt, ...)  ((void)0)
#define LOG_ERR(fmt, ...)  ((void)0)
#define LOG_DBG(fmt, ...)  ((void)0)
#else
#define LOCATE_ARGS(_module) _module, basename(const_cast<char*>(__FILE__)), __LINE__, __FUNCTION__
#define LOCATE_FORMAT "[%s](%s:%d)[%s]: "

// Format the timestamp once so a log line spanning a second boundary cannot mix fields from two seconds.
inline const char* log_time_str()
{
  static thread_local char buf[64];
  time_t now = time(NULL);
  struct tm tm_loc;
  localtime_r(&now, &tm_loc);
  snprintf(buf, sizeof(buf), "[%d-%d-%d/%02d:%02d:%02d]",
           tm_loc.tm_year + 1900, tm_loc.tm_mon + 1, tm_loc.tm_mday,
           tm_loc.tm_hour, tm_loc.tm_min, tm_loc.tm_sec);
  return buf;
}

#ifdef NOTIME
#define TIME_PREFIX
#define TIME_PREFIX_ARG
#else
#define TIME_PREFIX "%s"
#define TIME_PREFIX_ARG log_time_str(),
#endif

inline void logger(const char* fm, ...)
{
  va_list args;
  va_start(args, fm);
  va_list probe;
  va_copy(probe, args);
  int n = vsnprintf(nullptr, 0, fm, probe);
  va_end(probe);

  if (n > 0) {
    // Allocate by actual length so long log lines with full paths are not silently truncated.
    std::vector<char> buf((size_t)n + 1);
    vsnprintf(buf.data(), buf.size(), fm, args);
    LogOutputFunc sink = g_logOutputFunc.load(std::memory_order_acquire);
    if (sink) { sink(buf.data()); }
    printf("%s\n", buf.data());
    // Non-TTY stdout is fully buffered; flush explicitly so log lines appear promptly.
    fflush(stdout);
  }
  va_end(args);
}

#define LOG_INF(fmt, ...) logger(TIME_PREFIX "[INFO]" LOCATE_FORMAT fmt, TIME_PREFIX_ARG LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) logger(TIME_PREFIX "[WARN]" LOCATE_FORMAT fmt, TIME_PREFIX_ARG LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) logger(TIME_PREFIX "[ERROR]" LOCATE_FORMAT fmt, TIME_PREFIX_ARG LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) logger(TIME_PREFIX "[DEBUG]" LOCATE_FORMAT fmt, TIME_PREFIX_ARG LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#endif

#endif // FILE_TRANSFER_COMLOG_H
