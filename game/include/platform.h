#pragma once
#include<stdint.h>
#include<stdbool.h>
#include<assert.h>
#include"types.h"

C_BEGIN

#define PAGE_SIZE 0x1000

void platform_console_print(const char *ptr, size_t len);

void *platform_alloc_page_aligned(size_t size);
bool platform_free_page_aligned(void *ptr);

bool platform_init();

C_END
