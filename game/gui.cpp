#include"types.h"
#include"imgui.h"
#include"game.h"
#include"log.h"

/* 
 * Open ai chat wrote some of the code in this file...
 * Kinda, it took a lot of iteration and correction
 */

C_BEGIN

// Global variables for the FPS counter
static float fps_time = 0.0f;
static int fps_frames = 0;
static float last_fps = 0.0f;

const ImGuiWindowFlags gui_flags = ImGuiWindowFlags_NoDecoration |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;

// Function to calculate the FPS and update the FPS counter
void gui_FPS()
{
    fps_time += ImGui::GetIO().DeltaTime;
    fps_frames++;
    if (fps_time >= 1.0f)
    {
        // Calculate the FPS and reset the counters
        last_fps = fps_frames / fps_time;
        fps_time = 0.0f;
        fps_frames = 0;
    }
    // Start the FPS counter window
    ImGui::Begin("FPS Counter", NULL, gui_flags);

    // Set the window position
    ImGui::SetWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 90, 15), ImGuiCond_Always);
    ImGui::SetWindowSize(ImVec2(90, 20), ImGuiCond_Always);

    // Display the FPS value
    ImGui::Text("FPS: %.1f", last_fps);

    // End the FPS counter window
    ImGui::End();
}

void validate_params(GameParams *params)
{
    params->width = CLAMP(params->width, PARAMS_MIN_WIDTH, PARAMS_MAX_WIDTH);
    params->height = CLAMP(params->height, PARAMS_MIN_HEIGHT, PARAMS_MAX_HEIGHT);
    params->num_bombs = CLAMP(params->num_bombs, PARAMS_MIN_BOMBS, PARAMS_MAX_BOMBS);
    params->num_bombs = MIN(params->num_bombs, (params->height * params->width) - 1);
}

bool gui_difficulty(GameParams *params)
{
    bool ret = false;
    bool open_custom_popup = false;
    static GameParams custom_params = game_custom_default;

    if (ImGui::BeginPopupModal("CustomPopup", NULL, gui_flags)) {

        ImVec2 display_size = ImGui::GetIO().DisplaySize;
        ImGui::SetWindowPos(ImVec2(display_size.x/2, display_size.y/2), ImGuiCond_Always);
        ImGui::SetWindowSize(ImVec2(150, 150), ImGuiCond_Always);

          // unsigned int
        ImGui::BeginGroup();
        {
            if (ImGui::InputScalar("width", ImGuiDataType_U32, &custom_params.width)) {
                validate_params(&custom_params);
            }
            if (ImGui::InputScalar("height", ImGuiDataType_U32, &custom_params.height)) {
                validate_params(&custom_params);
            }
            if (ImGui::InputScalar("bombs", ImGuiDataType_U32, &custom_params.num_bombs)) {
                validate_params(&custom_params);
            }
        }
        ImGui::EndGroup();
        ImGui::BeginGroup();
        {
            if (ImGui::Button("Begin")) {
                validate_params(&custom_params);
                *params = custom_params;
                ret = true;
                ImGui::CloseCurrentPopup();
            }
            if (ImGui::Button("Cancel")) {
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndGroup();

        ImGui::EndPopup();
    }

    if (ImGui::BeginMainMenuBar()) {
        ImVec2 window_dims = ImGui::GetWindowSize();
        game_state.main_menu_bar_height_window_px = window_dims.y;//ImGui::GetFrameHeight();

        // Create the "Difficulty" menubar menu
        if (ImGui::BeginMenu("Difficulty"))
        {
            // Create the menu options
            if (ImGui::MenuItem("Easy")) {
                *params = game_easy;
                ret = true;
            }
            if (ImGui::MenuItem("Medium")) {
                *params = game_medium;
                ret = true;
            }
            if (ImGui::MenuItem("Hard")) {
                *params = game_hard;
                ret = true;
            }
            if (ImGui::MenuItem("Custom")) {
                open_custom_popup = true;
            }

            // End the "Difficulty" menubar menu
            ImGui::EndMenu();
        }

        // End the menubar
        ImGui::EndMainMenuBar();
    }

    if (open_custom_popup) {
        ImGui::OpenPopup("CustomPopup", ImGuiPopupFlags_NoOpenOverExistingPopup);
    }
    return ret;
}

C_END
