#ifndef _LOG_H_
#define _LOG_H_

#include<stdlib.h>
#include<stdio.h>
#include<stdarg.h>

#include"platform.h"

#define LOG_INFO 0
#define LOG_DEBUG 1
#define LOG_WARNING 2
#define LOG_ERROR 3

#ifdef __cplusplus
extern "C" {
#endif

void *log_buf;
#define LOG_BUF_SZ 0x1000

void _log_print(int level, const char *fmt, va_list args);

#define DEFINE_LOG_FN(_SUFFIX, _LEVEL) \
void log_##_SUFFIX(const char *fmt, ...) \
{ \
    va_list args;   \
    va_start(args, fmt);    \
    _log_print((_LEVEL), fmt, args);   \
    va_end(args); \
}

DEFINE_LOG_FN(info, LOG_INFO)
DEFINE_LOG_FN(debug, LOG_DEBUG)
DEFINE_LOG_FN(warn, LOG_WARNING)
DEFINE_LOG_FN(error, LOG_ERROR)

void _log_print(int level, const char *fmt, va_list args)
{
    ASSERT(log_buf);
    int result = vsnprintf((char* const)log_buf, LOG_BUF_SZ, fmt, args); 
    printf((const char* const)log_buf);
    if (result < 0) {
        log_error("Error printing log\n");
    } else if (result >= LOG_BUF_SZ) {
        log_error("Log was truncated\n");
    }
}

bool log_init()
{
    log_buf = platform_alloc_pages(1);
    if (!log_buf)
        return false;
    log_info("Log init success\n");
    return true;
}

#ifdef __cplusplus
}
#endif

#endif // _LOG_H_