#include<stdlib.h>
#include<stdio.h>
#include<stdarg.h>
#include<ctype.h>
#include<SDL.h>
#include"platform.h"
#include"log.h"

C_BEGIN

static void *log_buf;
#define LOG_BUF_SZ PAGE_SIZE

// timestamp should hold "XXXXXXXXXXXXdXXhXXmXX.XXXs"; enough for u64 ms
#define LOG_TIMESTAMP_SZ 30 // few extra just in case

static size_t u64toa(u64 num, char *buf)
{
    ASSERT(buf);

    if (num == 0) {
        *buf = '0';
        return 1;
    }

    char *start = buf;
    size_t len = 0;
    size_t base = 10;

    while (num) {
        u64 div = num / base;
        u64 rem = num - div * base;
        ASSERT(rem < base);
        *buf++ = '0' + (char)rem;
        num = div;
        len++;
    }
    buf--;
    while (start < buf) {
        char tmp = *buf;
        *buf-- = *start;
        *start++ = tmp;
    }

    return len;
}

static void append_time_field(u64 time, char suffix, char **buf, size_t *remaining, size_t *len)
{
    size_t n = u64toa(time, *buf);
    ASSERT(*remaining > n + 1);
    *remaining -= n + 1;
    *buf += n;
    **buf = suffix;
    *buf += 1; // note *buf++ would increment the char ** instead of the char *
    *len += n + 1;
}

static size_t format_time_ms(u64 ms, char* buf, size_t size)
{
    u64 secs = ms / 1000;
    u64 mins = secs / 60;
    u64 hours = mins / 60;
    u64 days = hours / 24;
    ms -= secs * 1000;
    secs -= mins * 60;
    mins -= hours * 60;
    hours -= days * 24;
    u64 fields[3] = {days, hours, mins};
    static const char names[3] = {'d', 'h', 'm'};

    size_t len = 0;
    size_t remaining = size;

    // d,h,m work the same
    for (int i = 0; i < 3; ++i) {
        if (fields[i] == 0) {
            continue;
        }
        append_time_field(fields[i], names[i], &buf, &remaining, &len);
    }

    // if secs is 0 we still want "0." before ms
    append_time_field(secs, '.', &buf, &remaining, &len);

    // if ms is 0 we still want "0s"
    append_time_field(ms, 's', &buf, &remaining, &len);
    ASSERT(remaining > 0);

    // in case we need it as a string
    *buf = '\0';

    return len;
}

static void _log_print(const char *str, size_t len)
{
    // TODO print to log file
    platform_console_print(str, len);
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

    size_t len = 0; // length of final string, including appended \n
    char* buf = (char *)log_buf;
    bool truncated = false;

    size_t remaining = LOG_BUF_SZ;
    *buf++ = '[';
    size_t time_len = format_time_ms(SDL_GetTicks64(), buf, LOG_TIMESTAMP_SZ);
    ASSERT(time_len < LOG_BUF_SZ - 3);
    buf += time_len;
    *buf++ = ']';
    *buf++ = ' ';
    remaining -= time_len + 3;
    len += time_len + 3;

    va_list args;
    va_start(args, fmt);
    int result = vsnprintf(buf, remaining, fmt, args);
    va_end(args);

    if (result > 0) {
        char *end;
        char *start;

        // Enforce single-line printing by removing \n's
        /*
         * if result >= remaining, vsnprintf truncated the string
         * but we want to add a \n on the end, so treat buffer as 1 char smaller
         */
        if (result >= remaining - 1) {
            truncated = true;
            len += remaining - 1;
            // overwrite the last char before the null
            end = &buf[remaining - 2];
        } else {
            // save the position of null char for \n to be inserted
            end = &buf[result];
            // append another null char after the existing one
            buf[result + 1] = '\0';
            // extend final string by the right amount
            len += result + 1;
        }
        // replace last char, or null char (depending on above)
        *end = '\n';
        // replace \n's with spaces, not including the final \n
        for (start = buf; start < end; ++start) {
            if (*start == '\n') {
                *start = ' ';
            }
        }

        _log_print(log_buf, len);

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
    // TODO
    // raw because SDL isn't initted yet
#ifdef DEBUG
    log_raw("Log init success\n");
#endif

    return true;
}

C_END
