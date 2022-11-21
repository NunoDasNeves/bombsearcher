#include<windows.h>
#include<stdint.h>
#include<stdbool.h>
#include"platform.h"

#ifdef __cplusplus
extern "C" {
#endif

void *platform_alloc_pages(uint32_t num)
{
    return VirtualAlloc(NULL, num*PAGE_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

bool platform_free_pages(void *ptr)
{
    return VirtualFree(ptr, 0, MEM_RELEASE);
}

bool platform_init()
{
    return true;
}

#ifdef __cplusplus
}
#endif