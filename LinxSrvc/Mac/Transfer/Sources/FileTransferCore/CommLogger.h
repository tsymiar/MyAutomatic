#ifndef FILE_TRANSFER_COMLOG_H
#define FILE_TRANSFER_COMLOG_H
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>

#ifndef LOG_TAG
#define LOG_TAG "Transfer"
#endif

#pragma GCC diagnostic ignored "-Wformat"

// ── Log output redirection ────────────────────────────────────────────────────
// Set by the C bridge (TransferBridge.cpp) to forward C++ logs to Swift/SwiftUI.
// When NULL (default), logs only go to stdout.
typedef void (*LogOutputFunc)(const char* msg);
extern LogOutputFunc g_logOutputFunc;

#ifdef NOLOG
#define LOG_INF(fmt, ...)  ((void)0)
#define LOG_WRN(fmt, ...)  ((void)0)
#define LOG_ERR(fmt, ...)  ((void)0)
#define LOG_DBG(fmt, ...)  ((void)0)
#else
#ifdef NOTIME
#define TIME_ARGS(_ptm)
#define TIME_FORMAT
#else
#define TIME_ARGS(_ptm) ((_ptm)->tm_year + 1900), ((_ptm)->tm_mon + 1), (_ptm)->tm_mday, (_ptm)->tm_hour, (_ptm)->tm_min, (_ptm)->tm_sec
#define TIME_FORMAT "[%d-%d-%d/%02d:%02d:%02d]"
#endif

#define LOCATE_ARGS(_module) _module, basename(const_cast<char*>(__FILE__)), __LINE__, __FUNCTION__
#define LOCATE_FORMAT "[%s](%s:%d)[%s]: "

inline struct tm* log_times() { static thread_local struct tm tm_loc; time_t now = time(NULL); localtime_r(&now, &tm_loc); return &tm_loc; }
inline void logger(const char* fm, ...)
{
  char buf[4096];
  va_list args;
  va_start(args, fm);
  int n = vsnprintf(buf, sizeof(buf), fm, args);
  va_end(args);

  if (n > 0) {
    if (g_logOutputFunc) { g_logOutputFunc(buf); }
    printf("%s\n", buf);
    // When stdout is not connected to a terminal, it defaults to fully buffered;
    // we must explicitly flush to ensure log lines are written out promptly.
    fflush(stdout);
  }
}

#define LOG_INF(fmt, ...) logger(TIME_FORMAT "[INFO]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) logger(TIME_FORMAT "[WARN]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) logger(TIME_FORMAT "[ERROR]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) logger(TIME_FORMAT "[DEBUG]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#endif

#endif // FILE_TRANSFER_COMLOG_H
