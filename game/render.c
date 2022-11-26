#include<stdlib.h>
#include"glad/glad.h"
#include"types.h"
#include"log.h"
#include"render.h"
#include"file.h"
#include"allocator.h"
#include"mem.h"

C_BEGIN

#define dump_errors() \
do {                                                \
    GLenum __err; \
    while ((__err = glGetError()) != GL_NO_ERROR) { \
        log_error("openGL error 0x%x", __err);      \
    }                                               \
} while(0)

// buffer for loading errors from opengl
#define INFO_LOG_SIZE 512
static char info_log[INFO_LOG_SIZE];

#define MAX_TEXTURES 256
typedef struct _Texture {
    GLuint id;
    GLuint fb_id;
    GLuint rb_id;
    u32 width;
    u32 height;
} Texture;
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

// 1x1 white texture
static Texture *empty_texture;

// TODO split into load and create
static GLuint create_shader(const char *filename, unsigned type)
{
    GLint success;
    GLuint id;
    u64 len = 0;
    GLchar const*shader_code;

    ASSERT(filename);

    log_debug("Loading shader \"%s\"", filename);

    // read in shader source
    shader_code = file_read(filename, &len, true);
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

static GLuint load_complete_shader(const char *vertex_filename, const char* fragment_filename)
{
    GLint success;
    GLuint vertex_id, fragment_id, program_id;

    vertex_id = create_shader(vertex_filename, GL_VERTEX_SHADER);
    if (!vertex_id) {
        return 0;
    }
    fragment_id = create_shader(fragment_filename, GL_FRAGMENT_SHADER);
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

    if (image_data != NULL) {
        /*
         * Note internal format ideally matches input format
         * here we use 8 bit normalized (GL_RGBA8), which means
         * it looks like a float in the shader, but is stored
         * as 4 8-bit integer (unsigned) components internally
         */
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8, // internal format
            width, height,
            0,
            GL_RGBA, GL_UNSIGNED_BYTE, // input format
            image_data);
        // we need to do this, even though we aren't using mipmaps
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    tex->width = width;
    tex->height = height;

    dump_errors();

    return tex;
}

bool render_init(GLADloadproc gl_get_proc_address, u32 width, u32 height)
{
    // Load OpenGL extensions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)gl_get_proc_address)) {
        log_error("Failed to initialize GLAD");
        return false;
    }

    screen.shader = load_complete_shader("shaders/screen.vert", "shaders/screen.frag");
    if (!screen.shader) {
        log_error("Failed to create screen shader");
        return false;
    }

    if (!pool_try_create(texture_pool, MAX_TEXTURES,
                         Texture, mem_alloc)) {
        return false;
    }

    glGenVertexArrays(1, &screen.vao);
    dump_errors();

    // create 1x1 white texture for default/untextured quads
    u8 buf[4] = {255, 255, 255, 255};
    empty_texture = create_texture(buf, 1, 1);

    return true;
}

C_END
