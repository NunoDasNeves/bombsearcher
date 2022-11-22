#pragma once
#include<stdint.h>
#include<stdbool.h>
#include<assert.h>
#include"types.h"

C_HEADER_START

#define PAGE_SIZE 0x1000

void *platform_alloc_pages(uint32_t num);
bool platform_free_pages(void *ptr);

bool platform_init();

C_HEADER_END