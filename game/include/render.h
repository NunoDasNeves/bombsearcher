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

static Color color_rgb8(u8 r, u8 g, u8 b)
{
    Color c = {{r/255.0f, g/255.0f, b/255.0f, 1.0f}};
    return c;
}
static Color color_red()
{
    Color c = {{1, 0, 0, 1}};
    return c;
}
static Color color_none()
{
    Color c = {{0, 0, 0, 0}};
    return c;
}

#define COLOR_RGB8(r,g,b) \
    {.data = {(r)/255.0f, (g)/255.0f, (b)/255.0f, 1.0f}}

typedef struct {
    GLuint id;
    GLuint fb_id;
    GLuint rb_id;
    u32 width;
    u32 height;
} glTexture;
extern glTexture *empty_texture;

void shader_set_texture(GLuint shader_id, glTexture* texture);
void shader_set_color(GLuint shader_id, Color color);
void shader_set_transform_pixels(GLuint shader_id, f32 width, f32 height);

glTexture *create_texture(void* image_data, u32 width, u32 height);
glTexture *load_texture(const char* filename);

void render_start(Color color);
void render_end();

void render_resize_window(u32 width, u32 height);

bool render_init(GLADloadproc gl_get_proc_address, u32 width, u32 height);

#define dump_errors() \
do {                                                \
    GLenum __err; \
    while ((__err = glGetError()) != GL_NO_ERROR) { \
        log_error("openGL error 0x%x", __err);      \
    }                                               \
} while(0)

C_END
