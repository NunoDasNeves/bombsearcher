#pragma once
#include"glad/glad.h"
#include"types.h"
C_BEGIN

/*
 * Basic unlit flat shader stuff
 */
extern GLuint shader_flat;

typedef union {
    struct {
        f32 r;
        f32 g;
        f32 b;
        f32 a;
    };
    f32 data[4];
} Color;

typedef struct {
    GLuint id;
    GLuint fb_id;
    GLuint rb_id;
    u32 width;
    u32 height;
} Texture;
extern Texture *empty_texture;

void shader_set_texture(GLuint shader_id, Texture* texture);

Texture *create_texture(void* image_data, u32 width, u32 height);
Texture *load_texture(const char* filename);

void render_start(Color color);
void render_end();

void render_resize(u32 width, u32 height);

bool render_init(GLADloadproc gl_get_proc_address, u32 width, u32 height);

#define dump_errors() \
do {                                                \
    GLenum __err; \
    while ((__err = glGetError()) != GL_NO_ERROR) { \
        log_error("openGL error 0x%x", __err);      \
    }                                               \
} while(0)

C_END
