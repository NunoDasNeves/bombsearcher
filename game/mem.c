#include<stddef.h>
#include<string.h>
#include"types.h"
#include"platform.h"
#include"mem.h"
#include"log.h"
#include"allocator.h"
#ifdef _WIN32
typedef struct {
    void* x;
} max_align_t;
#endif
#include<stdalign.h>
#define max_alignment alignof(max_align_t)

C_BEGIN

static u64 allocated; // allocated - freed
static u64 footprint; // allocated

static unsigned current_context;

static struct {
    struct BumpAllocator bumps[MEM_SCRATCH_BUFFERS];
    unsigned current_scope;
    u64 allocated;
    u64 footprint;
} scratch;

static struct {
    struct PoolAllocator pools[MEM_LONGTERM_BUCKET_MAX_POW - MEM_LONGTERM_BUCKET_MIN_POW];
    u64 allocated;
    u64 footprint;
} longterm;

static struct {
    struct BumpAllocator bump;
    u64 allocated;
    u64 footprint;
} nofree;

unsigned mem_set_context(unsigned type)
{
    // TODO
    return current_context;
}

unsigned mem_get_current_context()
{
    return current_context;
}

unsigned mem_get_context(void * ptr)
{
    if (ptr >= nofree.bump.base && (char *)ptr < (char *)nofree.bump.base + nofree.bump.size) {
        return MEM_CTX_NOFREE;
    }

    // TODO

    return MEM_CTX_OTHER;
}

unsigned mem_scratch_scope_begin()
{
    // TODO
    return 0;
}

unsigned mem_scratch_scope_end()
{
    // TODO
    return 0;
}

unsigned mem_get_scratch_scope()
{
    // TODO
    return 0;
}

void *mem_alloc(u64 size)
{
    switch (current_context) {
        case MEM_CTX_NOFREE:
        {
            return mem_alloc_nofree(size);
        }
        case MEM_CTX_SCRATCH:
        {
            return NULL;
        }
        case MEM_CTX_LONGTERM:
        {
            return NULL;
        }
        default:
        {
            log_error("mem_context is invalid: %u", current_context);
        }
    }
    return NULL;
}

void *mem_alloc_aligned(u64 align, u64 size)
{
    if (align < sizeof(void*) || !IS_POW_2(align)) {
        return NULL;
    }
    // check size is a  multiple of align
    if (size & (align-1)) {
        return NULL;
    }
    // TODO depend on allocator - mem_alloc does extra work
    size += align - 1;
    void *ret = mem_alloc(size);
    ret = (void *)ALIGN_UP_POW_2(ret, align);

    return ret;
}

void *mem_realloc_sized(void *ptr, u64 oldsize, u64 newsize)
{
    if (ptr == NULL) {
        return mem_alloc(newsize);
    }
    if (newsize == 0) {
        mem_free(ptr);
        return NULL;
    }

    if (mem_get_context(ptr) == MEM_CTX_LONGTERM) {
        // TODO
        return NULL;
    }

    if (newsize <= oldsize) {
        return ptr;
    }
    // otherwise naive copy
    void * new_ptr = mem_alloc(newsize);
    memcpy(new_ptr, ptr, oldsize);

    return new_ptr;
}

void mem_free(void *ptr)
{
    if (current_context != MEM_CTX_LONGTERM) {
        return;
    }
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

void mem_free_longterm(void *ptr)
{
    // TODO
}

void *mem_alloc_nofree(u64 size)
{
    ASSERT(nofree.bump.base);

    u64 align = max_alignment;
    ASSERT(align >= sizeof(void*));
    ASSERT((size + align - 1) > size);

    size += align - 1;
    void *ret = bump_alloc(&nofree.bump, size);
    ret = (void*)ALIGN_UP_POW_2(ret, align);

    if (ret != NULL) {
        allocated += size;
        footprint += size;
        nofree.allocated += size;
        nofree.footprint += size;
    }

    log_debug("total allocated: %u", allocated);

    return ret;
}

void mem_get_allocated(u64 *ret_allocated, u64 *ret_footprint)
{
    ASSERT(ret_allocated);
    ASSERT(ret_footprint);

    // TODO contexts

    *ret_allocated = allocated;
    *ret_footprint = footprint;
}

bool mem_init(u64 mem_budget)
{
    void *nofree_base = platform_alloc_page_aligned(mem_budget);
    if (!nofree_base) {
        log_error("Alloc nofree buffer");
        return false;
    }
    if (!bump_init_allocator(&nofree.bump, nofree_base, mem_budget)) {
        log_error("Init nofree allocator");
        return false;
    }

    // TODO MEM_CTX_SCRATCH, MEM_CTX_LONGTERM

    current_context = MEM_CTX_NOFREE;
    allocated = 0;
    footprint = 0;

    return true;
}

C_END
