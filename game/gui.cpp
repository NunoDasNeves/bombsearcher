#include"types.h"
#include"imgui.h"
#include"game.h"
#include"log.h"
#include"mem.h"

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

void gui_debug()
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

    u64 allocated, footprint;
    mem_get_allocated(&allocated, &footprint);

    ImGui::Begin("Debug Diag", NULL, gui_flags);

    ImGui::SetWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 150, 15), ImGuiCond_Always);
    //ImGui::SetWindowSize(ImVec2(90, 20), ImGuiCond_Always);

    ImGui::Text("FPS: %.1f", last_fps);

    ImGui::Text("Alloc: %lukB", allocated/1000);

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

    //ImGui::ShowStyleEditor();
    static const ImVec4 dark_grey = ImVec4(0.54F, 0.54F, 0.54F, 1.0F);
    static const ImVec4 mid_grey = ImVec4(0.65F, 0.65F, 0.65F, 1.0F);
    static const ImVec4 light_grey = ImVec4(0.82F, 0.82F, 0.82F, 1.0F);
    static const struct {
        ImGuiCol idx;
        ImVec4 col;
    } style[] = {
        {ImGuiCol_WindowBg, mid_grey},
        {ImGuiCol_MenuBarBg, mid_grey},
        {ImGuiCol_HeaderHovered, light_grey},
        {ImGuiCol_PopupBg, mid_grey},
        {ImGuiCol_Button, light_grey},
        {ImGuiCol_ButtonHovered, light_grey},
        {ImGuiCol_ButtonActive, dark_grey},
        {ImGuiCol_Text, ImVec4(0,0,0,1)},
        {ImGuiCol_TextSelectedBg, ImVec4(0.2f,0.3f,0.8f,0.5f)},
        {ImGuiCol_FrameBg, ImVec4(1,1,1,1)},
    };

    for (u32 i = 0; i < ARRAY_LEN(style); ++i) {
        ImGui::PushStyleColor(style[i].idx, style[i].col);
    }

    if (ImGui::BeginPopupModal("CustomPopup", NULL, gui_flags)) {

        ImGui::SetWindowSize(ImVec2(183, 165), ImGuiCond_Always);
        ImGui::SetWindowPos(ImVec2(25, 45), ImGuiCond_Always);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15,10));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(8,4));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20,20));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8,8));
        ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1);

        if (ImGui::InputScalar("width", ImGuiDataType_U32, &custom_params.width)) {
            validate_params(&custom_params);
        }
        if (ImGui::InputScalar("height", ImGuiDataType_U32, &custom_params.height)) {
            validate_params(&custom_params);
        }
        if (ImGui::InputScalar("bombs", ImGuiDataType_U32, &custom_params.num_bombs)) {
            validate_params(&custom_params);
        }
        if (ImGui::Button("Begin")) {
            validate_params(&custom_params);
            *params = custom_params;
            ret = true;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();
        ImGui::PopStyleVar();

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

    for (u32 i = 0; i < ARRAY_LEN(style); ++i) {
        ImGui::PopStyleColor();
    }

    if (open_custom_popup) {
        ImGui::OpenPopup("CustomPopup", ImGuiPopupFlags_NoOpenOverExistingPopup);
    }
    return ret;
}

C_END
