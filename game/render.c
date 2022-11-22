#include"glad/glad.h"
#include"log.h"
#include"types.h"
#include"render.h"

bool render_init(GLADloadproc gl_get_proc_address, u32 width, u32 height)
{
    // Load OpenGL extensions with GLAD
    if (!gladLoadGLLoader((GLADloadproc)gl_get_proc_address))
    {
        log_error("Failed to initialize GLAD");
        return false;
    }

    return true;
}