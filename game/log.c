#include<stdlib.h>
#include<stdio.h>
#include<stdarg.h>
#include<ctype.h>

#include"platform.h"
#include"log.h"

static void *log_buf;
#define LOG_BUF_SZ PAGE_SIZE

static char *log_level_strings[] = {
    "INFO",
    "DEBUG",
    "WARN",
    "ERROR"
};

static int format_time(char *buf, u64 time_ms)
{
    // TODO
    return 0;
}

static void _log_print(const char *str, size_t len)
{
    // TODO print to log file, use platform primitives
    printf(str);
}

/* Don't do anything fancy; print exactly what is asked for */
void log_raw(const char *fmt, ...)
{
    ASSERT(fmt);

    char* buf = (char *)log_buf;

    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buf, LOG_BUF_SZ, fmt, args);
    va_end(args);

    if (result > 0) {
        size_t len = result >= LOG_BUF_SZ ? LOG_BUF_SZ - 1 : result;
        _log_print(buf, len);
    } else {
        log_error("%s: formatting failed", __func__);
    }
}

/*
 * Print with timestamp, append an \n, only print one line (replace \n's with spaces)
 */
void log_timestamp(const char *fmt, ...)
{
    ASSERT(fmt);
    ASSERT(log_buf);

    char* buf = (char *)log_buf;
    bool truncated = false;

    // TODO timestamp

    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buf, LOG_BUF_SZ, fmt, args);
    va_end(args);

    if (result > 0) {
        char *end;
        char *start;
        size_t len; // length of final string, including appended \n

        // Enforce single-line printing by removing \n's
        /*
         * if result >= LOG_BUF_SZ, vsnprintf truncated the string
         * but we want to add a \n on the end, so treat buffer as 1 char smaller
         */
        if (result >= LOG_BUF_SZ - 1) {
            truncated = true;
            len = LOG_BUF_SZ - 1;
            // overwrite the last char before the null
            end = &buf[LOG_BUF_SZ - 2];
        } else {
            // extend string by 1 char for \n
            len = result + 1;
            // append another null char
            buf[len] = '\0';
            // replace first null char
            end = &buf[result];
        }
        *end = '\n';
        // replace \n's with spaces, not including the final \n
        for (start = buf; start < end; ++start) {
            if (*start == '\n') {
                *start = ' ';
            }
        }

        _log_print(buf, len);

    } else {
        log_warn("%s: formatting failed", __func__);
    }
    if (truncated) {
        log_warn("%s: string was truncated", __func__);
    }
}

bool log_init()
{
    log_buf = platform_alloc_page_aligned(LOG_BUF_SZ);
    if (!log_buf) {
        return false;
    }
    // raw because SDL isn't initted yet
    log_raw("Log init success\n");

    return true;
}