#pragma once
#include"types.h"

C_BEGIN

typedef union {
    struct {
        f32 x,y;
    };
    f32 data[2];
} Vec2f;

static inline Vec2f vec2f(f32 x, f32 y)
{
    Vec2f v = {{x,y}};
    return v;
}

C_END
