#include"glad/glad.h"
#include"log.h"
#include"types.h"
#include"render.h"
#include"file.h"

#define dump_errors() \
do {                                                                \
    GLenum _err;                                                    \
    while ((_err = glGetError()) != GL_NO_ERROR) {                  \
        log_error("GL error 0x%x", _err, __FUNCTION__, __LINE__);   \
    }                                                               \
} while(0)

// buffer for loading errors from opengl
#define INFO_LOG_SIZE 512
static char info_log[INFO_LOG_SIZE];

// TODO
static GLuint screen_vao; // we need to bind some vao to draw the screen triangle...

static GLuint create_shader(const char *filename, unsigned type)
{
    GLint success;
    GLuint id;
    u64 len = 0;
    char *shader_code;

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

static GLuint create_vertex_shader(const char *filename)
{
    return create_shader(filename, GL_VERTEX_SHADER);
}

static GLuint create_fragment_shader(const char *filename)
{
    return create_shader(filename, GL_FRAGMENT_SHADER);
}

static GLuint load_complete_shader(const char *vertex_filename, const char* fragment_filename)
{
    GLint success;
    GLuint vertex_id, fragment_id, program_id;

    vertex_id = create_vertex_shader(vertex_filename);
    if (!vertex_id) {
        return 0;
    }
    fragment_id = create_fragment_shader(fragment_filename);
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

bool render_init(GLADloadproc gl_get_proc_address, u32 width, u32 height)
{
    // Load OpenGL extensions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)gl_get_proc_address)) {
        log_error("Failed to initialize GLAD");
        return false;
    }

    GLuint screen_shader_id = load_complete_shader("shaders/screen.vert", "shaders/screen.frag");
    if (!screen_shader_id) {
        log_error("Failed to create screen shader");
        return false;
    }

    return true;
}