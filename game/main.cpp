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

/*
 * Note on setting stbi allocators
 * stbi allocs and frees everything within the stbi_load_* variants,
 * it doesn't maintain any state, so it's safe to use with scratch/scoped allocators
 * So we give it the generic context-sensitive mem_alloc()/mem_free(),
 * that way we control the context of that memory and when it's freed
 */
#define STBI_MALLOC(sz) mem_alloc(sz)
#define STBI_REALLOC_SIZED(p,oldsz,newsz) mem_realloc_sized(p,oldsz,newsz)
#define STBI_FREE(p) mem_free(p)
#define STBI_ASSERT(X) ASSERT(X)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool keep_running = true;

/* return how much it's scaled down in power of 2 */
u32 resize_window_to_game(u32 desired_width, u32 desired_height)
{
    SDL_Window *window = SDL_GL_GetCurrentWindow();
    SDL_DisplayMode mode;
    int top, left, bot, right;

    if (SDL_GetDesktopDisplayMode(0, &mode) < 0)
    {
        log_error("Couldn't get desktop display mode: %s", SDL_GetError());
        // default to something smallish
        mode.w = 1280;
        mode.h = 720;
        mode.refresh_rate = 60;
    }
    log_debug("Desktop display mode %d x %d %dHz", mode.w, mode.h, mode.refresh_rate);

    if (SDL_GetWindowBordersSize(window, &top, &left, &bot, &right) < 0)
    {
        log_error("Couldn't get window border size: %s", SDL_GetError());
        top = left = bot = right = 0;
    }
    log_debug("Window borders: %d %d %d %d", top, left, bot, right);

    /*
     * Reduce by window borders, then multiply by 2/3
     * Should give an estimate of how much space it's ok to occupy
     * ...Hopefully
     */
    u32 max_w = ((mode.w - left - right) * 4) / 5;
    u32 max_h = ((mode.h - top - bot) * 4) / 5;
    log_debug("Max size to occupy: %u %u", max_w, max_h);

    log_debug("Desired game dims %u %u", desired_width, desired_height);

    u32 window_w = desired_width;
    u32 window_h = desired_height;
    u32 scale = 0;

    while (window_w > max_w || window_h > max_h) {
        window_w >>= 1;
        window_h >>= 1;
        scale++;
    }
    window_w = MAX(window_w, desired_width>>2);
    window_h = MAX(window_h, desired_height>>2);

    log_debug("Resizing window to %u %u", window_w, window_h);
    SDL_SetWindowSize(window, (int)window_w, (int)window_h);

    desired_width = window_w;
    desired_height = window_h;
    return scale;
}

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
                case SDL_WINDOWEVENT_SIZE_CHANGED:
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
            u32 mouse_x = (u32)e->motion.x;
            u32 mouse_y = (u32)e->motion.y;
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        button_down = true;
        case SDL_MOUSEBUTTONUP:
        {
            if (imgui_io.WantCaptureMouse) {
                log_error("imgui wants mouse");
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
            switch (keycode) {
                case SDLK_r:
                    break;
            }
            break;
        }
    }
}

static Input poll_mouse(ImGuiIO& imgui_io)
{
    static Input input = {}; // static so we can use same input again if imgui takes control
    i32 window_pixel_x, window_pixel_y;
    u32 state;

    if (imgui_io.WantCaptureMouse) {
        input.mouse_left_down = false;
        input.mouse_right_down = false;
        return input;
    }

    state = SDL_GetMouseState(&window_pixel_x, &window_pixel_y);

    input.mouse_left_down = state & SDL_BUTTON_LMASK;
    input.mouse_right_down = state & SDL_BUTTON_RMASK;
    input.mouse_x = (u32)window_pixel_x;
    input.mouse_y = (u32)window_pixel_y;

    return input;
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

    SDL_SetRelativeMouseMode(SDL_FALSE);

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
        Input input = poll_mouse(imgui_io);

        if (!game_update_and_render(input)) {
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
