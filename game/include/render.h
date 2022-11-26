#pragma once
#include"glad/glad.h"
#include"types.h"
C_BEGIN

typedef union _Color {
    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
    f32 data[4];
} Color;

void render_start(Color color);
void render_end();

void render_resize(u32 width, u32 height);

bool render_init(GLADloadproc gl_get_proc_address, u32 width, u32 height);

C_END
