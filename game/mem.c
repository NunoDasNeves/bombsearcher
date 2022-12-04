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

static mem_ctx_t current_context;

static struct {
    BumpAllocator bumps[MEM_SCRATCH_BUFFERS];
    int current_scope;
    u64 allocated[MEM_SCRATCH_BUFFERS];
    u64 footprint[MEM_SCRATCH_BUFFERS];
} scratch;

#define NUM_POOLS ((MEM_LONGTERM_BUCKET_MAX_POW) - (MEM_LONGTERM_BUCKET_MIN_POW))
static struct {
    struct PoolAllocator pools[NUM_POOLS];
    u64 allocated[NUM_POOLS];
    u64 footprint[NUM_POOLS];
} longterm;

static struct {
    BumpAllocator bump;
    u64 allocated;
    u64 footprint;
} nofree;

mem_ctx_t mem_set_context(mem_ctx_t type)
{
    mem_ctx_t ctx = current_context;
    if (type >= 0 && type < MEM_CTX_OTHER) {
        current_context = type;
    }
    return ctx;
}

mem_ctx_t mem_get_current_context()
{
    return current_context;
}

mem_ctx_t mem_get_context(void * ptr)
{
    if (ptr >= nofree.bump.base && (char *)ptr < (char *)nofree.bump.base + nofree.bump.size) {
        return MEM_CTX_NOFREE;
    }

    // TODO

    return MEM_CTX_OTHER;
}

int mem_scratch_scope_begin()
{
    ASSERT(scratch.current_scope >= MEM_SCRATCH_SCOPE_NONE);
    ASSERT(scratch.current_scope < MEM_SCRATCH_SCOPE_MAX);

    if (scratch.current_scope == MEM_SCRATCH_SCOPE_MAX) {
        log_error("Scratch scope is max! This is probably very bad");
        return scratch.current_scope;
    }

    scratch.current_scope++;
    return scratch.current_scope;
}

int mem_scratch_scope_end()
{
    ASSERT(scratch.current_scope >= MEM_SCRATCH_SCOPE_NONE);
    ASSERT(scratch.current_scope <= MEM_SCRATCH_SCOPE_MAX);

    if (scratch.current_scope < 0) {
        return MEM_SCRATCH_SCOPE_NONE;
    }
    bump_reset(&scratch.bumps[scratch.current_scope]);
    // bookkeeping
    allocated -= scratch.allocated[scratch.current_scope];
    scratch.allocated[scratch.current_scope] = 0;
    if (scratch.current_scope >= 0) {
        scratch.current_scope--;
    }
    return scratch.current_scope;;
}

int mem_get_scratch_scope()
{
    ASSERT(scratch.current_scope >= -1);
    ASSERT(scratch.current_scope < MEM_SCRATCH_BUFFERS);

    return scratch.current_scope;;
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
            return mem_alloc_scratch(size);
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

void *mem_alloc_longterm(u64 size)
{
    // TODO
    return NULL;
}

void mem_free_longterm(void *ptr)
{
    // TODO
}

static void *mem_alloc_bump(BumpAllocator *bump, u64 size,
                            u64 *b_allocated, u64 *b_footprint)
{
    ASSERT(bump->base);

    u64 align = max_alignment;
    ASSERT(align >= sizeof(void*));
    ASSERT((size + align - 1) > size);

    size += align - 1;
    void *ret = bump_alloc(bump, size);
    ret = (void*)ALIGN_UP_POW_2(ret, align);

    if (ret != NULL) {
        allocated += size;
        footprint += size;
        *b_allocated += size;
        *b_footprint += size;
    }

    log_debug("total allocated: %u", allocated);

    return ret;
}

void *mem_alloc_scratch(u64 size)
{
    int idx = scratch.current_scope;
    if (idx < 0) {
        return NULL;
    }
    if (idx >= MEM_SCRATCH_BUFFERS) {
        idx = MEM_SCRATCH_BUFFERS - 1;
    }
    return mem_alloc_bump(&scratch.bumps[idx],
                          size,
                          &scratch.allocated[idx],
                          &scratch.footprint[idx]);
}

void *mem_alloc_nofree(u64 size)
{
    return mem_alloc_bump(&nofree.bump, size,
                          &nofree.allocated, &nofree.footprint);
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

    if (!bump_try_create(nofree.bump, mem_budget, platform_alloc_page_aligned)) {
        return false;
    }

    scratch.current_scope = MEM_SCRATCH_SCOPE_NONE;
    for (u32 i = 0; i < MEM_SCRATCH_BUFFERS; ++i) {
        if (!bump_try_create(scratch.bumps[i], mem_budget, platform_alloc_page_aligned)) {
            log_error("Failed on scratch #%u", i);
            return false;
        }
    }

    // TODO MEM_CTX_LONGTERM

    current_context = MEM_CTX_NOFREE;
    allocated = 0;
    footprint = 0;

    return true;
}

C_END
