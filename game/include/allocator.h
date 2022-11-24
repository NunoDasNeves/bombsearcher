/*
 * Basic allocator implementations
 */
#pragma once
#include"types.h"
#include"log.h"

C_BEGIN

struct BumpAllocator {
    void *base;
    void *next_free;
    u64 size;
};

static bool bump_init_allocator(struct BumpAllocator *allocator, void *mem, u64 size)
{
    ASSERT(allocator);
    ASSERT(mem);

    allocator->base = mem;
    allocator->next_free = mem;
    allocator->size = size;

    return true;
}

static void *bump_alloc(struct BumpAllocator *allocator, u64 req_size)
{
    ASSERT(allocator);
    ASSERT(allocator->base);
    ASSERT(allocator->next_free);

    void *ret = allocator->next_free;
    char *base = (char*)allocator->base;
    char *new_free = base + req_size;
    if (new_free > base + allocator->size) {
        return NULL;
    }
    allocator->next_free = new_free;
    return ret;
}

static void bump_reset(struct BumpAllocator *allocator)
{
    ASSERT(allocator);
    ASSERT(allocator->base);
    ASSERT(allocator->next_free);

    allocator->next_free = allocator->base;
}

static bool bump_allocator_test()
{
    // TODO
}

struct PoolAllocator {
    void *base; // base should be aligned to block_size
    void *next_free;
    u64 size;
    u64 block_size;
};

static bool pool_init_allocator(struct PoolAllocator *allocator, void *mem, u64 size, u64 block_size)
{
    ASSERT(allocator);
    ASSERT(mem);

    // TODO better checks, maybe
    if (block_size < sizeof(void*)) {
        return false;
    }
    if ((size % block_size) != 0) {
        return false;
    }
    if (((u64)mem % block_size) != 0) {
        return false;
    }

    allocator->base = mem;
    allocator->next_free = mem;
    allocator->size = size;
    allocator->block_size = block_size;

    /* point each node in the free list to the next block */
    char *curr = (char*)allocator->base;
    char *last_element = (char*)allocator->base + size - block_size;
    for(; curr < last_element; curr += block_size) {
        void **free_list_node = (void **)curr;
        *free_list_node = curr + block_size;
    }

    return true;
}

static void *pool_alloc(struct PoolAllocator *allocator)
{
    ASSERT(allocator);
    ASSERT(allocator->base);
    ASSERT(allocator->block_size >= sizeof(void*));

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
    ASSERT(allocator->block_size >= sizeof(void*));

    if (ptr < allocator->base || (char*)ptr >= (char*)allocator->base + allocator->size) {
        log_warn("Pool tried to free invalid pointer");
        return;
    }

    void **free_list_node = (void **)ptr;
    *free_list_node = allocator->next_free;
    allocator->next_free = ptr;
}

static bool pool_allocator_test()
{
    // TODO
}

C_END
