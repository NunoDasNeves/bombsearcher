#include<stdlib.h>
#include<unistd.h>
#include"types.h"
#include"platform.h"
#include"log.h"

C_BEGIN

void platform_console_print(const char *ptr, size_t len)
{
    write(1, ptr, len);
}

/* TODO maybe use memmap and madvise*/
void *platform_alloc_page_aligned(size_t size)
{
    return valloc(size);
}

bool platform_free_page_aligned(void *ptr)
{
    ASSERT((void *)ALIGN_UP(ptr, PAGE_SIZE) == ptr);
    free(ptr);
    return true;
}

bool platform_init()
{
    ASSERT(sysconf(_SC_PAGE_SIZE) == PAGE_SIZE);
    return true;
}

C_END
