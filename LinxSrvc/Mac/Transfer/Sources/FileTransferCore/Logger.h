#ifndef FILE_TRANSFER_LOGGER_H
#define FILE_TRANSFER_LOGGER_H
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <libgen.h>

#ifndef LOG_TAG
#define LOG_TAG "Transfer"
#endif

#pragma GCC diagnostic ignored "-Wformat"

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

inline struct tm* log_times() { time_t now = time(NULL); static struct tm* local = NULL; local = localtime(&now); return local; }
inline void logger(const char* fm, ...)
{
  va_list args;
  va_start(args, fm);
  static_cast<void>(vprintf(fm, args));
  va_end(args);
  static_cast<void>(printf("\n"));
}

#define LOG_INF(fmt, ...) logger(TIME_FORMAT "[INFO]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) logger(TIME_FORMAT "[WARN]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) logger(TIME_FORMAT "[ERROR]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#define LOG_DBG(fmt, ...) logger(TIME_FORMAT "[DEBUG]" LOCATE_FORMAT fmt, TIME_ARGS(log_times()), LOCATE_ARGS(LOG_TAG), ##__VA_ARGS__)
#endif

#endif // FILE_TRANSFER_LOGGER_H
