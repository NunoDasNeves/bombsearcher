#include<stdlib.h>
#include<stdio.h>
#include<stdarg.h>
#include<ctype.h>

#include"platform.h"
#include"log.h"

static void *log_buf;
#define LOG_BUF_SZ 0x1000

static char *log_level_strings[] = {
    "INFO",
    "DEBUG",
    "WARN",
    "ERROR"
};

static void _log_print(const char *fmt, va_list args);

/* Don't do anything fancy; print exactly what is asked for */
void log_raw(const char *fmt, ...)
{
    ASSERT(fmt);
    va_list args;
    va_start(args, fmt);
    _log_print(fmt, args);
    va_end(args);
}

/*
 * Fancy printing with log level, file, line number, on a single line
 */
void log_level(unsigned level, const char* source_file, unsigned line_number, const char *fmt, ...)
{
    va_list args;
    char* buf = (char *)log_buf;

    ASSERT(log_buf);
    ASSERT(fmt);
    ASSERT(level <= LOG_LEVEL_MAX);
    // TODO timestamp
    int result = snprintf(
        buf,
        LOG_BUF_SZ,
        "%s:%s:%u -- %s\n",
        log_level_strings[level],
        source_file,
        line_number,
        fmt);
    if (result > 0) {
        // process the string, removing newlines (except the one we added above)
        // get position of null char in resulting string
        char *end = result >= LOG_BUF_SZ - 1 ? &buf[LOG_BUF_SZ - 1] : &buf[result];
        end--; // skip the null char
        // replace \n with space
        while (end > buf) {
            end--; // do this first to skip the last newline char we added above
            if (*end == '\n') {
                *end = ' ';
            }
        }

        va_start(args, fmt);
        _log_print(buf, args);
        va_end(args);
    }

    if (result < 0) {
        log_error("Log error: %s formatting failed", __func__);
    } else if (result >= LOG_BUF_SZ) {
        log_error("Log error: %s string was truncated", __func__);
    }
}

static void _log_print(const char *fmt, va_list args)
{
    char* buf = (char *)log_buf;

    ASSERT(log_buf);
    ASSERT(fmt);
    int result = vsnprintf(buf, LOG_BUF_SZ, fmt, args); 
    printf(buf);
    // TODO print to log file
    if (result < 0) {
        log_error("Log error: %s formatting failed", __func__);
    } else if (result >= LOG_BUF_SZ) {
        log_error("Log error: %s string was truncated", __func__);
    }
}

bool log_init()
{
    log_buf = platform_alloc_page_aligned(PAGE_SIZE);
    if (!log_buf)
        return false;
    log_info("Log init success");
    return true;
}