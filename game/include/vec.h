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

static inline Vec2f vec2f_add(Vec2f a, Vec2f b)
{
    Vec2f v = {{a.x + b.x, a.y + b.y}};
    return v;
}

static inline Vec2f vec2f_sub(Vec2f a, Vec2f b)
{
    Vec2f v = {{a.x - b.x, a.y - b.y}};
    return v;
}

C_END
