#pragma once
#include"types.h"
#include"platform.h"

C_HEADER_START

/*
 * Allocator design
 * Goals:
 * - Pre-allocate entire memory budget at startup, no platform_alloc_* after alloc_init() is done
 * - Rely on OS to allocate physical pages when they are touched
 * - Static memory budget, probably (could maybe detect it...)
 * - Context-aware and context-unaware allocations - some code will know it needs short term, but maybe not
 * - Fast
 * - Easy to use
 * 
 * Allocation scenarios
 * - Init; memory that will never be freed
 *   - bump allocator will do, never reset it
 *   - we may want to do short-term and long-term allocations during init though!
 * - Short-term; memory that will be freed within a frame boundary or within some defined scope like after init
 *   - strings - paths, intermediate strings while building longer strings
 *   - file data that will be deserialized
 *   - scratch buffers for processing by systems (e.g. collisions, physics)
 *   - bump allocator/s able to reset
 *   - may want to nest these for different scopes - e.g. frame, system, function, etc...
 * - Long-term; memory that will possibly be freed and reused at some point
 *   - assets - meshes / textures / binary file data
 *   - strings - names, labels, GUI text strings
 *   - entitys/components
 *   - other game state; levels, etc...
 *   - pool allocator for fixed-size, freeable stuff like game state/objects
 *     - pool would actually be created from Init memory
 *   - may need flexible alloc + free sometimes
 *     - could use free list or buddy system
 *     - or something like array of pools for different sizes
 *       - e.g. pools of size 8, 16, 32, 64 ... 1GiB, each pool having like 100 GiB of (virtual) memory
 *       - round up allocation request and choose pool of that size
 *       - hope pools don't get too holey (same problem with other kinds of allocation though)
 *       - should be very fast; allocate and free in constant time
 *       - don't need to iterate over them, just need to store free list pointers
 *       - will take ages to init small pools (13400 million free list pointers), maybe make those smaller
 * 
 * So the idea is, allocate large regions of virtual memory for each allocator, let the OS populate them as they are used
 * If lots of memory moves from one allocator to another, we might have more pages allocated than we are really using
 * In that case, may have to use platform primitives to free underlying memory... but we'll see
 */

enum {
    MEM_INIT = 0,   /* never freed */
    MEM_SCRATCH,    /* short term; scoped; nested; can only free the whole scope */
    MEM_LONGTERM   /* long term; freeable */
};

/* sets context, returns the previous context (so you can restore it later if you want) */
unsigned mem_set_context(unsigned type); // MEM_* enum
unsigned mem_current_context();
unsigned mem_get_context(void * ptr);

/* For creating pools for longterm allocations. Doubling from min to max, inclusive */
#define MEM_LONGTERM_BUCKET_MIN 8
#define MEM_LONGTERM_BUCKET_MAX GiB(1)

#define MEM_SCRATCH_BUFFERS 4
/*
 * Increment the scratch scope, moving to a new scratch buffer if the resulting scope is
 * less than MEM_SCRATCH_BUFFERS
 * return the new scope
 */
unsigned mem_scratch_begin();
/*
 * Decrement the scratch scope by 1, freeing the current scratch buffer if the previous
 * scope was less than MEM_SCRATCH_SCOPES
 * return the new scope
 */
unsigned mem_scratch_end();

/*
 * context-dependent allocation - can be used in place of malloc/free
 * These will use init, scratch (in the current scratch scope) or longterm depending on context
 */
void *mem_alloc(u64 size);
void mem_free(); // does nothing in scratch context

/*
 * Allocate from the current scratch buffer
 * To free the current scratch buffer, use mem_scratch_end()
 */
void *mem_alloc_scratch(u64 size);
/*
 * Allocate from longerm memory
 */
void *mem_alloc_longterm(u64 size);
void mem_free_longterm();

/*
 * Allocate virtual addresses for mem_budget*(1 + MEM_LONGTERM_BUCKETS + MEM_SCRATCH_BUFFERS) and set up buffers
 * mem_budget should be a multiple of MEM_LONGTERM_BUCKETS_MAX
 * Context is set to MEM_INIT, scratch scope is 0
 */
bool mem_init(u64 mem_budget);

C_HEADER_END