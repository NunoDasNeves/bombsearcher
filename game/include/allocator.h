/*
 * Basic allocator implementations
 */
#pragma once
#include"types.h"
#include"log.h"

C_BEGIN

typedef struct {
    void *base;
    void *next_free;
    u64 size;
} BumpAllocator;

static bool bump_init_allocator(BumpAllocator *allocator, void *mem, u64 size)
{
    ASSERT(allocator);
    ASSERT(mem);

    allocator->base = mem;
    allocator->next_free = mem;
    allocator->size = size;

    return true;
}

static void *bump_alloc(BumpAllocator *allocator, u64 req_size)
{
    ASSERT(allocator);
    ASSERT(allocator->base);
    ASSERT(allocator->next_free);

    char *base = (char *)allocator->base;
    char *next_free = (char *)allocator->next_free;
    void *ret = next_free;
    char *new_free = next_free + req_size;
    if (new_free > base + allocator->size) {
        return NULL;
    }
    allocator->next_free = new_free;
    return ret;
}

static void bump_reset(BumpAllocator *allocator)
{
    ASSERT(allocator);
    ASSERT(allocator->base);
    ASSERT(allocator->next_free);

    allocator->next_free = allocator->base;
}

static bool _bump_try_create(BumpAllocator *bump,
                            size_t size, void *(*alloc_fn)(size_t),
                            char *name)
{
    ASSERT(bump);
    ASSERT(alloc_fn);
    ASSERT(name);

    void *mem = alloc_fn(size);

    if (mem == NULL) {
        log_error("Failed to alloc %s", name);
        return false;
    }
    if (!bump_init_allocator(bump, mem, size)) {
        log_error("Failed to init %s", name);
        return false;
    }
    return true;
}

#define bump_try_create(bump, size, alloc_fn) \
    _bump_try_create(&bump, size, alloc_fn, #bump)



static bool bump_allocator_test()
{
    // TODO
    return true;
}

struct PoolAllocator {
    void *base; // base should be aligned to obj_size
    void *next_free;
    u64 size;
    u64 obj_size;
};

static bool pool_init_allocator(struct PoolAllocator *allocator, void *mem, u64 size, u64 obj_size)
{
    ASSERT(allocator);
    ASSERT(mem);

    // TODO better checks, maybe
    if (obj_size < sizeof(void*)) {
        log_error("obj_size too small");
        return false;
    }
    if ((size % obj_size) != 0) {
        log_error("size not multiple of obj_size");
        return false;
    }
    // assume mem is aligned well-enough

    allocator->base = mem;
    allocator->next_free = mem;
    allocator->size = size;
    allocator->obj_size = obj_size;

    /* point each node in the free list to the next block */
    char *curr = (char*)allocator->base;
    char *last_element = (char*)allocator->base + size - obj_size;
    for(; curr < last_element; curr += obj_size) {
        void **free_list_node = (void **)curr;
        *free_list_node = curr + obj_size;
    }

    return true;
}

static void *pool_alloc(struct PoolAllocator *allocator)
{
    ASSERT(allocator);
    ASSERT(allocator->base);
    ASSERT(allocator->obj_size >= sizeof(void*));

    void *ret = allocator->next_free;

    if (ret == NULL) {
        return NULL;
    }

    void **free_list_node = (void **)ret;
    allocator->next_free = *free_list_node;

    return ret;
}

static void pool_free(struct PoolAllocator *allocator, void *ptr)
{
    ASSERT(allocator);
    ASSERT(allocator->base);
    ASSERT(allocator->obj_size >= sizeof(void*));

    if (ptr < allocator->base || (char*)ptr >= (char*)allocator->base + allocator->size) {
        log_warn("Pool tried to free invalid pointer");
        return;
    }

    void **free_list_node = (void **)ptr;
    *free_list_node = allocator->next_free;
    allocator->next_free = ptr;
}

static bool _pool_try_create(struct PoolAllocator *pool,
                            size_t num_objs, size_t obj_size,
                            void *(*alloc_fn)(size_t), char *name)
{
    ASSERT(pool);
    ASSERT(alloc_fn);
    ASSERT(name);

    size_t mem_size = num_objs*obj_size;
    void *mem = alloc_fn(mem_size);

    if (mem == NULL) {
        log_error("Failed to alloc %s", name);
        return false;
    }
    if (!pool_init_allocator(pool, mem, mem_size, obj_size)) {
        log_error("Failed to init %s", name);
        return false;
    }
    return true;
}

#define pool_try_create(pool, num_objs, type, alloc_fn) \
    _pool_try_create(&pool, num_objs, sizeof(type), alloc_fn, #pool)

static bool pool_allocator_test()
{
    // TODO
    return true;
}

C_END
