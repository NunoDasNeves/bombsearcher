#pragma once
#include"types.h"

C_BEGIN

/*
 * len should be 0 to read entire file, otherwise len bytes read
 * length of data read returned in len
 * if to_string returned buffer is a C string (size len + 1)
 */
char *file_read(const char *filename, u64 *len, bool to_string);

static char *file_read_to_string(const char *filename, u64 *len)
{
    *len = 0;
    return file_read(filename, len, true);
}

unsigned char *image_file_read(const char *filename, u64 *size, u32 *width, u32 *height);

C_END
