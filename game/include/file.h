#pragma once
#include"types.h"

C_HEADER_START

/*
 * len should be 0 to read entire file
 * length of data read returned in len
 * if to_string returned buffer is a C string (size len + 1)
 */
char *file_read(const char *filename, u64 *len, bool to_string);

C_HEADER_END