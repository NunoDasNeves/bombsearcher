#include<windows.h>
#include"types.h"
#include"platform.h"
#include"log.h"

C_BEGIN

void *platform_alloc_page_aligned(size_t size)
{
    return VirtualAlloc(NULL, ALIGN_UP(size, PAGE_SIZE), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

bool platform_free_page_aligned(void *ptr)
{
    ASSERT((void *)ALIGN_UP(ptr, PAGE_SIZE) == ptr);
    return VirtualFree(ptr, 0, MEM_RELEASE);
}

bool platform_init()
{
    return true;
}

C_END
