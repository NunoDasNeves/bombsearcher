#include"types.h"
#include"platform.h"
#include"mem.h"

unsigned mem_set_context(unsigned type)
{
    // TODO
    return 0;
}

unsigned mem_current_context()
{
    // TODO
    return 0;
}

unsigned mem_get_context(void * ptr)
{
    // TODO
    return 0;
}

unsigned mem_scratch_begin()
{
    // TODO
    return 0;
}

unsigned mem_scratch_end()
{
    // TODO
    return 0;
}

void *mem_alloc(u64 size)
{
    // TODO
    return NULL;
}
void mem_free()
{
    // TODO
}

void *mem_alloc_scratch(u64 size)
{
    // TODO
    return NULL;
}

void *mem_alloc_longterm(u64 size)
{
    // TODO
    return NULL;
}

void mem_free_longterm()
{
    // TODO
}

bool mem_init(u64 mem_budget)
{
    // TODO
    return false;
}