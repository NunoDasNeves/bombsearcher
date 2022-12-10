#pragma once
#include<string.h>
#include"types.h"
#include"mem.h"
#include"log.h"

C_BEGIN

typedef struct {
    void *data;
    u64 capacity;
    u32 obj_size;
    u64 len;
} Array;

bool array_init(Array *a, u64 len, u32 obj_size)
{
    ASSERT(a);
    ASSERT(obj_size <= UINT64_MAX/len);

    u64 size = obj_size * len;
    a->data = mem_alloc(size);
    if (!a->data) {
        log_error("Failed to alloc Array");
        return false;
    }
    a->capacity = len;
    a->len = 0;
    a->obj_size = obj_size;

    return true;
}

bool _try_alloc_array(Array *a, u64 len, u32 obj_size, const char* name)
{
    if (!array_init(a, len, obj_size)) {
        log_error("Failed to alloc array \"%s\"", name);
        return false;
    }
    return true;
}

#define try_alloc_array(arr, num_objs, type) \
    _try_alloc_array(&arr, (num_objs), sizeof(type), #arr)

bool array_extend(Array *a, u64 num)
{
    ASSERT(a);
    ASSERT(a->data);
    ASSERT(a->capacity >= a->len);
    ASSERT(a->len + num >= num);

    u64 remaining_capacity = a->capacity - a->len;
    if (remaining_capacity < num) {
        u64 new_capacity = a->capacity + (num - remaining_capacity);
        a->data = mem_realloc_sized(a->data, a->capacity * a->obj_size, new_capacity * a->obj_size);
        if (!a->data) {
            log_error("Failed to extend Array");
            return false;
        }
        a->capacity = new_capacity;
    }

    return true;
}

// Append num objs, extending as needed
bool array_append(Array *a, void *objs, u64 num)
{
    ASSERT(a);
    ASSERT(a->data);
    ASSERT(objs);
    ASSERT(a->obj_size <= UINT64_MAX/num);
    ASSERT(a->capacity >= a->len);

    if (!array_extend(a, num)) {
        return false;
    }

    char *buf = (char *)a->data;
    memcpy(&buf[a->len * a->obj_size], objs, num * a->obj_size);
    a->len += num;

    return true;
}

void array_clear(Array *a)
{
    ASSERT(a);
    a->len = 0;
}

// clear array, and fill with num copies of single value pointed to by obj
bool array_fill(Array *a, void *obj, u64 num)
{
    ASSERT(a);
    ASSERT(a->data);
    ASSERT(obj);
    ASSERT(a->obj_size <= UINT64_MAX/num);
    ASSERT(a->capacity >= a->len);

    array_clear(a);
    if (!array_extend(a, num)) {
        return false;
    }

    a->len = num;
    char* buf = a->data;

    while (num > 0) {
        num--;
        memcpy(buf, obj, a->obj_size);
        buf += a->obj_size;
    }

    return true;
}

C_END