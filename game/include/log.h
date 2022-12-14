#pragma once
#include"types.h"
C_BEGIN

enum {
    LOG_INFO = 0,
    LOG_DEBUG,
    LOG_WARN,
    LOG_ERROR,
    LOG_LEVEL_MAX = LOG_ERROR
};

void log_raw(const char *fmt, ...);
void log_timestamp(const char *fmt, ...);
bool log_init();

#define log_fmt(level,fmt) #level ":" __FILE__ ":" TO_STRING(__LINE__) " -- " fmt

#define log_info(fmt, ...) \
    log_timestamp(log_fmt(INFO, fmt), ##__VA_ARGS__)
#define log_warn(fmt, ...) \
    log_timestamp(log_fmt(WARN, fmt), ##__VA_ARGS__)
#define log_error(fmt, ...) \
    log_timestamp(log_fmt(ERROR, fmt), ##__VA_ARGS__)

#ifdef DEBUG
#define log_debug(fmt, ...) \
    log_timestamp(log_fmt(DEBUG, fmt), ##__VA_ARGS__)
#else
#define log_debug(fmt, ...)
#endif

#define CHECK_LOG(expr, ret, fmt, ...) \
    do {    \
        if (!(expr)) {  \
            log_error(fmt, ##__VA_ARGS__); \
            return (ret);   \
        }   \
    } while (0);

#define CHECKV_LOG(expr, ret, fmt, ...) \
    do {    \
        if (!(expr)) {  \
            log_error(fmt, ##__VA_ARGS__); \
            return (ret);   \
        }   \
    } while (0);

C_END
