#pragma once
#include"types.h"
C_HEADER_START

enum {
    LOG_INFO = 0,
    LOG_DEBUG,
    LOG_WARN,
    LOG_ERROR,
    LOG_LEVEL_MAX = LOG_ERROR
};

void log_raw(const char *fmt, ...);
void log_level(unsigned level, const char* source_file, unsigned line_number, const char *fmt, ...);
bool log_init();

#define log_info(...) \
    log_level(LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) \
    log_level(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...) \
    log_level(LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) \
    log_level(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

C_HEADER_END