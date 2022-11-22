#pragma once
#include"types.h"
#include"platform.h"

C_HEADER_START

// TODO figure out what to dooo
enum {
    ALLOC_SHORT_TERM,
    ALLOC_LONG_TERM
};

extern struct AllocContext {
    int type;
} alloc_context;

void *alloc(size_t size);
void free(void *ptr);

C_HEADER_END