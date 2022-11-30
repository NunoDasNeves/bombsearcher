#include<SDL.h>
#include"glad/glad.h"
#include"imgui.h"
#include"imgui_impl_sdl.h"
#include"imgui_impl_opengl3.h"

#include"types.h"
#include"platform.h"
#include"log.h"
#include"mem.h"
#include"render.h"
#include"game.h"

#define STBI_MALLOC(sz) mem_alloc(sz)
#define STBI_REALLOC_SIZED(p,oldsz,newsz) mem_realloc_sized(p,oldsz,newsz)
#define STBI_FREE(p) mem_free(p)
#define STBI_ASSERT(X) ASSERT(X)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool keep_running = true;

static void handle_event(SDL_Window* window, SDL_Event* e, ImGuiIO& imgui_io)
{
    bool button_down = false;

    switch(e->type)
    {
        case SDL_QUIT:
        {
            keep_running = false;
            break;
        }
        case SDL_WINDOWEVENT:
        {
            switch(e->window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                {
                    int width, height;
                    SDL_GL_GetDrawableSize(window, &width, &height);
                    render_resize(width, height);
                    break;
                }
            }
            break;
        }
        case SDL_MOUSEMOTION:
        {
            if (imgui_io.WantCaptureMouse) {
                break;
            }
            f32 mouse_motion_x = (f32)e->motion.xrel;
            f32 mouse_motion_y = (f32)e->motion.yrel;
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
        {
            if (imgui_io.WantCaptureMouse) {
                break;
            }
            switch(e->button.button)
            {
                case SDL_BUTTON_LEFT:
                {
                    bool mouse_left_down = button_down;
                    break;
                }
                case SDL_BUTTON_RIGHT:
                {
                    bool mouse_right_down = button_down;
                    break;
                }
            }
            break;
        }
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
            if (imgui_io.WantCaptureKeyboard) {
                break;
            }
            SDL_Keycode keycode = e->key.keysym.sym;
            break;
        }
    }
}

static void poll_mouse(ImGuiIO& imgui_io)
{
    if (imgui_io.WantCaptureMouse) {
        return;
    }
    i32 window_pixel_x, window_pixel_y;
    SDL_GetMouseState(&window_pixel_x, &window_pixel_y);
}

int main(int argc, char **argv)
{
    SDL_Window* window = NULL;
    SDL_GLContext gl_context = NULL;
    SDL_GameController* controller = NULL;

    if (!platform_init()) {
        return EXIT_FAILURE;
    }

    if (!log_init()) {
        return EXIT_FAILURE;
    }

    if (!mem_init(GiB(1))) {
        log_error("Failed to initialize memory subsystem");
        return EXIT_FAILURE;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) {
        log_error("SDL couldn't be initialized - SDL_Error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    window = SDL_CreateWindow(
        "Game",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        INIT_GAME_WINDOW_WIDTH, INIT_GAME_WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL);
    
    if(window == NULL) {
        log_error("Window could not be created - SDL_Error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Initialize openGL context
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, PLATFORM_GL_MAJOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, PLATFORM_GL_MINOR_VERSION);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    gl_context = SDL_GL_CreateContext(window);
    if(gl_context == NULL) {
        log_error("OpenGL context could not be created - SDL_Error: %s", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Use Vsync
    if(SDL_GL_SetSwapInterval(1) < 0)
    {
        log_error("Warning: Unable to set VSync! SDL Error: %s", SDL_GetError());
    }

    // Init IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& imgui_io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);

    const char* glsl_version = "#version 130";
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load OpenGL extensions with GLAD
    if (!render_init((GLADloadproc)SDL_GL_GetProcAddress, INIT_GAME_WINDOW_WIDTH, INIT_GAME_WINDOW_HEIGHT)) {
        log_error("Failed to initialize render");
        return EXIT_FAILURE;
    }

    // Detect controller
    int num_joysticks = SDL_NumJoysticks();
    if (num_joysticks > 0) {
        if (SDL_IsGameController(0)) {
            controller = SDL_GameControllerOpen(0);
            if (controller) {
                log_debug("Opened a controller");
            } else {
                log_debug("Couldn't open joystick 0");
            }
        }
    } else {
        log_info("No joysticks found");
    }

    if (!game_init()) {
        log_error("Failed to initialize game");
        return EXIT_FAILURE;
    }

    SDL_SetRelativeMouseMode(SDL_TRUE);

    SDL_Event e;

    while(keep_running) {
        /* starting the imgui frame before processing input seems to be easiest way to get imgui_io.WantCaptureMouse etc to actually work, contrary to the examples */
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        bool imgui_used_event = false;
        while (SDL_PollEvent(&e))
        {
            imgui_used_event = ImGui_ImplSDL2_ProcessEvent(&e);
            handle_event(window, &e, imgui_io);
        }
        poll_mouse(imgui_io);

        if (!game_update_and_render()) {
            keep_running = false;
        }

        ImGui::Render();
        glViewport(0, 0, (int)imgui_io.DisplaySize.x, (int)imgui_io.DisplaySize.y);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap buffers (actually make the image appear)
        SDL_GL_SwapWindow(window);


    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
