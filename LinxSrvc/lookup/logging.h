#ifndef LOGGING_H
#define LOGGING_H
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#ifndef LOG_TAG
#define LOG_TAG "logging"
#endif
#ifdef _WIN32
inline const char* basename(const char* name)
{
    char file[256];
    size_t len = 256;
    snprintf(file, len, "%s", name);
    static char* base = new char[len];
    for (size_t i = 0; i < len; i++) { if (file[i] == '/' || file[i] == '\\') { memcpy(base, file + i + 1, len - i); memset(base + len - i, 0, 1); } }
    return base;
}
#else
#include <libgen.h>
//#pragma GCC diagnostic ignored "-Wwritable-strings"
#pragma GCC diagnostic ignored "-Wformat"
#endif
#ifdef __ANDROID__
#include <android/log.h>
#ifdef __cplusplus
#define _LOG_(level, fmt, ...) __android_log_print(level, LOG_TAG,"(%s:%d)[%s]: " fmt, basename(const_cast<char*>(__FILE__)), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define _LOG_(level, fmt, ...) __android_log_print(level, LOG_TAG,"(%s:%d)[%s]: " fmt, basename(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__)
#endif
#define LOG_DBG(fmt, ...) _LOG_(ANDROID_LOG_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INF(fmt, ...) _LOG_(ANDROID_LOG_INFO, fmt, ##__VA_ARGS__)
#define LOG_WRN(fmt, ...) _LOG_(ANDROID_LOG_WARN, fmt, ##__VA_ARGS__)
#define LOG_ERR(fmt, ...) _LOG_(ANDROID_LOG_ERROR, fmt, ##__VA_ARGS__)
#else
#ifdef NOTIME
#define TIME_ARGS(_ptm)
#define TIME_FORMAT
#else
#define TIME_ARGS(_ptm) ((_ptm)->tm_year + 1900), ((_ptm)->tm_mon + 1), (_ptm)->tm_mday, (_ptm)->tm_hour, (_ptm)->tm_min, (_ptm)->tm_sec
#define TIME_FORMAT "[%d-%d-%d/%02d:%02d:%02d]"
#endif //NOTIME
#ifdef NOLOCATE
#define LOCATE_ARGS
#define LOCATE_FORMAT
#else
#define LOCATE_ARGS(_module) _module,basename(const_cast<char*>(__FILE__)),__LINE__,__FUNCTION__
#define LOCATE_FORMAT "[%s](%s:%d)[%s]: "
#endif //NOLOCATE
inline struct tm* times() { time_t now = time(NULL); static struct tm* local = NULL; local = localtime(&now); return local; }
inline void logger(const char* fm, ...) { va_list args; va_start(args, fm); static_cast<void>(vprintf(fm, args)); va_end(args); static_cast<void>(printf("\n")); }
#define LOG_INF(fmt, ...) logger(TIME_FORMAT "[INFO]" LOCATE_FORMAT fmt, TIME_ARGS(times()), LOCATE_ARGS(LOG_TAG),##__VA_ARGS__)
#define LOG_WRN(fmt, ...) logger(TIME_FORMAT "[WARN]" LOCATE_FORMAT fmt, TIME_ARGS(times()), LOCATE_ARGS(LOG_TAG),##__VA_ARGS__)
#define LOG_ERR(fmt, ...) logger(TIME_FORMAT "[ERROR]" LOCATE_FORMAT fmt, TIME_ARGS(times()), LOCATE_ARGS(LOG_TAG),##__VA_ARGS__)
#define LOG_DBG(fmt, ...) logger(TIME_FORMAT "[DEBUG]" LOCATE_FORMAT fmt, TIME_ARGS(times()), LOCATE_ARGS(LOG_TAG),##__VA_ARGS__)
#endif //ANDROID
#endif //LOGGING_H
