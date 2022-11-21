#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include<stdint.h>
#include<stdbool.h>
#include<assert.h>

#define PAGE_SIZE 0x1000

#ifdef DEBUG
#define ASSERT(_EXP) assert(_EXP)
#else
#define ASSERT(_EXP) (_EXP)
#endif

#ifdef __cplusplus
extern "C" {
#endif

void *platform_alloc_pages(uint32_t num);
bool platform_free_pages(void *ptr);

bool platform_init();

#ifdef __cplusplus
}
#endif

#endif // _PLATFORM_H_