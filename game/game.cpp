#include<SDL.h>
#include"glad/glad.h"
#include"imgui.h"
#include"imgui_impl_sdl.h"
#include"imgui_impl_opengl3.h"
#include"stb_image.h"

#include"types.h"
#include"platform.h"
#include"log.h"
#include"render.h"

bool game_update_and_render()
{
    render_start({0,0,0,1});
    render_end();
    return true;
}

bool game_init()
{
    return true;
}
