#include<stdlib.h>
#include"glad/glad.h"
#include"types.h"
#include"log.h"
#include"render.h"
#include"file.h"
#include"allocator.h"
#include"mem.h"
#include"matrix.h"

C_BEGIN

// buffer for loading errors from opengl
#define INFO_LOG_SIZE 512
static char info_log[INFO_LOG_SIZE];

#define MAX_TEXTURES 256
struct PoolAllocator texture_pool;

/*
 * Screen triangle
 * We render everything to a framebuffer texture, then draw it
 * to a single triangle that covers the screen.
 * This will let us do...stuff. Later.
 */
static struct {
    // we need to bind a vao to draw the screen triangle...
    GLuint vao;
    GLuint shader;
    Texture *texture; // framebuffer texture
} screen;

GLuint shader_flat;

// 1x1 white texture
Texture *empty_texture;

static GLint gl_viewport_x = 0;
static GLint gl_viewport_y = 0;
static GLsizei gl_viewport_width = 0;
static GLsizei gl_viewport_height = 0;
static f32 viewport_aspect = 0;

void shader_set_color(GLuint shader_id, Color color)
{
    GLint loc;

    glUseProgram(shader_id);
    loc = glGetUniformLocation(shader_id, "color_blend");
    glUniform4fv(loc, 1, color.data);

    dump_errors();
}

void shader_set_texture(GLuint shader_id,
                        Texture* texture)
{
    GLint loc;

    glUseProgram(shader_id);

    loc = glGetUniformLocation(shader_id, "tex");
    glUniform1i(loc, 0); // put 0 into uniform sampler, corresponding to TEXTURE0
    glActiveTexture(GL_TEXTURE0); // texture unit 0 (whose value is not 0)
    glBindTexture(GL_TEXTURE_2D, texture->id); // bind to texture unit 0

    dump_errors();
}

static void shader_set_transform(GLuint shader_id,
                                 const Mat4 *projection,
                                 const Mat4 *view,
                                 const Mat4 *model)
{
    GLint loc;

    glUseProgram(shader_id);

    loc = glGetUniformLocation(shader_id, "projection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, projection->data);

    loc = glGetUniformLocation(shader_id, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, view->data);

    loc = glGetUniformLocation(shader_id, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, model->data);

    dump_errors();
}

void shader_set_transform_pixels(GLuint shader_id, f32 width, f32 height)
{
    log_debug("Resizing ortho transform (%f, %f)", width, height);
    // we want the origin to be in the top left
    Mat4 proj_matrix = mat4_ortho(0, width,  // left at 0, right at pixel width
                                  height, 0, // bottom at height, top at 0, so y=0 == height, y=height == 0
                                  -1, 1);
    Mat4 view_matrix = mat4_ident();
    Mat4 model_matrix = mat4_ident();
    shader_set_transform(shader_id,
                         &proj_matrix,
                         &view_matrix,
                         &model_matrix);
}

// TODO split into load and create
static GLuint load_shader(const char *filename, unsigned type)
{
    GLint success;
    GLuint id;
    u64 len;
    GLchar const*shader_code;

    ASSERT(filename);

    log_debug("Loading shader \"%s\"", filename);

    // read in shader source
    shader_code = file_read_to_string(filename, &len);
    if (!shader_code) {
        log_error("Failed to load shader source");
        return 0;
    }

    // create shader objects
    id = glCreateShader(type);

    // compile the source, and check if compilation was successful
    glShaderSource(id, 1, &shader_code, NULL);
    glCompileShader(id);
    glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(success != GL_TRUE) {
        glGetShaderInfoLog(id, INFO_LOG_SIZE, NULL, info_log);
        log_error("Shader compilation failed:");
        log_raw("%s", info_log);
        glDeleteShader(id);
        return 0;
    }
    return id;
}

static GLuint create_shader_program(const char *vertex_filename, const char* fragment_filename)
{
    GLint success;
    GLuint vertex_id, fragment_id, program_id;

    vertex_id = load_shader(vertex_filename, GL_VERTEX_SHADER);
    if (!vertex_id) {
        return 0;
    }
    fragment_id = load_shader(fragment_filename, GL_FRAGMENT_SHADER);
    if (!fragment_id) {
        glDeleteShader(vertex_id);
        return 0;
    }

    // Create and link the shader program which uses these shaders
    program_id = glCreateProgram();
    glAttachShader(program_id, vertex_id);
    glAttachShader(program_id, fragment_id);
    glLinkProgram(program_id);

    // Check for errors
    glGetProgramiv(program_id, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program_id, INFO_LOG_SIZE, NULL, info_log);
        log_error("Shader program linking failed:");
        log_raw("%s", info_log);
        glDeleteShader(vertex_id);
        glDeleteShader(fragment_id);
        glDeleteProgram(program_id);
        return 0;
    }

    // After the program is linked we don't need these anymore
    glDeleteShader(vertex_id);
    glDeleteShader(fragment_id);

    dump_errors();
    return program_id;
}

Texture* create_texture(void* image_data, u32 width, u32 height)
{
    Texture* tex = pool_alloc(&texture_pool);

    if (tex == NULL) {
        return NULL;
    }

    tex->id = 0;
    tex->fb_id = 0;
    tex->rb_id = 0;

    // Create and load texture
    glGenTextures(1, &tex->id);
    glBindTexture(GL_TEXTURE_2D, tex->id);

    /*
     * Note these affect the bound texture
     */
    // TODO maybe don't use GL_NEAREST, maybe clamp it
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    /*
        * Note internal format ideally matches input format
        * here we use 8 bit normalized (GL_RGBA8), which means
        * it looks like a float in the shader, but is stored
        * as 4 8-bit integer (unsigned) components internally
        */
    glTexImage2D(
        GL_TEXTURE_2D,
        0, // mipmap level
        GL_RGBA8, // internal format
        width, height,
        0, // border. has to be 0
        GL_RGBA, GL_UNSIGNED_BYTE, // input format
        image_data); // it's ok if image_data is NULL; the buffer will still be created
    // we need to do this, even though we aren't using mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);

    tex->width = width;
    tex->height = height;

    dump_errors();

    return tex;
}

static Texture* create_fb_texture(u32 width, u32 height)
{
    Texture* tex = create_texture(NULL, width, height);
    if (!tex) {
        return NULL;
    }

    // create frame buffer
    glGenFramebuffers(1, &tex->fb_id);
    glBindFramebuffer(GL_FRAMEBUFFER, tex->fb_id);

    // already done in create_texture
    // create buffer for texture data
    //glBindTexture(GL_TEXTURE_2D, tex->id);
    // NULL for data because we haven't rendered yet
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
    //             width, height, 0,
    //             GL_RGBA, GL_UNSIGNED_BYTE,
    //             NULL);

    // attach it to the framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, tex->id,
                           0); // level

    /*
     * render buffers contain an image
     * err, they are optimized for rendering better
     * than textures when don't need to resample after
     * rendering
     */
    // create renderbuffer for depth and stencil
    glGenRenderbuffers(1, &tex->rb_id);
    glBindRenderbuffer(GL_RENDERBUFFER, tex->rb_id);
    // analogous to glTexImage2D - create buffer of this size
    glRenderbufferStorage(GL_RENDERBUFFER,
                          GL_DEPTH24_STENCIL8,
                          width, height);
    // attach to the framebuffer for drawing, for depth and stencil both
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, tex->rb_id);

    // check it worked?
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        log_error("Failed to create frame buffer");
        // TODO cleanup
    }

    // unbind stuff - not needed
    //glBindTexture(GL_TEXTURE_2D, 0);
    //glBindRenderbuffer(GL_RENDERBUFFER, 0);
    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    dump_errors();

    return tex;
}

Texture *load_texture(const char* filename)
{
    u64 size;
    mem_ctx_t mem_ctx;
    u32 width, height;
    unsigned char *image_data;
    Texture *tex;

    ASSERT(filename);

    log_debug("Loading texture \"%s\"", filename);

    MEM_SCRATCH_START(mem_ctx);

    // read in shader source
    image_data = image_file_read(filename, &size, &width, &height);
    if (image_data == NULL) {
        log_error("Failed to load texture \"%s\"", filename);
        MEM_SCRATCH_END(mem_ctx);
        return NULL;
    }

    tex = create_texture(image_data, width, height);

    MEM_SCRATCH_END(mem_ctx);

    return tex;
}

static void render_resize_screen_tex(u32 width, u32 height)
{
    ASSERT(screen.texture);

    log_debug("Resizing screen texture (%d, %d)", width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, screen.texture->fb_id);

    /* resize texture and renderbuffer */
    glBindTexture(GL_TEXTURE_2D, screen.texture->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindRenderbuffer(GL_RENDERBUFFER, screen.texture->rb_id);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    dump_errors();
}

void render_resize_window(u32 width, u32 height)
{
    // TODO check glGetIntegerv(GL_MAX_VIEWPORT_DIMS, GLint *max_dims);
    viewport_aspect = (f32)width / (f32)height;
    gl_viewport_width = width;
    gl_viewport_height = height;
    glViewport(gl_viewport_x, gl_viewport_y, gl_viewport_width, gl_viewport_height);
    dump_errors();
    log_debug("Resizing gl viewport (%d, %d)", gl_viewport_width, gl_viewport_height);

    render_resize_screen_tex(width, height);
}

/* render screen triangle */
void render_end()
{
    ASSERT(screen.texture != NULL);

    shader_set_texture(screen.shader, screen.texture);

    /* set default framebuffer - the one that will display in the viewport */
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(1, 0, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // reset stuff for screen shader
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    // Set current shader program
    glUseProgram(screen.shader);

    glBindVertexArray(screen.vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    dump_errors();
}

void render_start(Color color)
{
    assert(screen.texture != NULL);

    /* set the screen frame buffer - we want to draw to the screen texture */
    glBindFramebuffer(GL_FRAMEBUFFER, screen.texture->fb_id);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shader_flat);

    dump_errors();
}

bool render_init(GLADloadproc gl_get_proc_address, u32 width, u32 height)
{
    // Load OpenGL extensions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)gl_get_proc_address)) {
        log_error("Failed to initialize GLAD");
        return false;
    }

    mem_set_context(MEM_CTX_SCRATCH);
    ASSERT(mem_scratch_scope_begin() == 0);

    screen.shader = create_shader_program("shaders/screen.vert", "shaders/screen.frag");
    if (!screen.shader) {
        log_error("Failed to create screen shader");
        mem_scratch_scope_end();
        mem_set_context(MEM_CTX_NOFREE);
        return false;
    }

    shader_flat = create_shader_program("shaders/flat.vert", "shaders/flat.frag");
    if (!shader_flat) {
        log_error("Failed to create flat shader");
        mem_scratch_scope_end();
        mem_set_context(MEM_CTX_NOFREE);
        return false;
    }

    ASSERT(mem_scratch_scope_end() == -1);
    mem_set_context(MEM_CTX_NOFREE);

    if (!pool_try_create(texture_pool, MAX_TEXTURES,
                         Texture, mem_alloc)) {
        return false;
    }

    glGenVertexArrays(1, &screen.vao);
    dump_errors();

    // create 1x1 white texture for default/untextured quads
    u8 buf[4] = {255, 255, 255, 255};
    empty_texture = create_texture(buf, 1, 1);
    shader_set_texture(shader_flat, empty_texture);

    // texture for the screen triangle, size to the screen
    screen.texture = create_fb_texture(width, height);
    shader_set_texture(screen.shader, screen.texture);

    return true;
}

C_END
