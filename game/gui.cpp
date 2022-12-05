#include"types.h"
#include"imgui.h"
#include"game.h"
#include"log.h"

/* 
 * Open ai chat wrote most of the code in this file...
 * Kinda, it took a lot of iteration and correction
 */

C_BEGIN

// Global variables for the FPS counter
static float fps_time = 0.0f;
static int fps_frames = 0;
static float last_fps = 0.0f;

const ImGuiWindowFlags gui_flags = ImGuiWindowFlags_NoTitleBar |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoCollapse |
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

void gui_difficulty()
{
    if (ImGui::BeginMainMenuBar())
    {

        game_state.main_menu_bar_height_window_px = ImGui::GetFrameHeight();

        // Create the "Difficulty" menubar menu
        if (ImGui::BeginMenu("Difficulty"))
        {
            // Create the menu options
            if (ImGui::MenuItem("Easy")) {}
            if (ImGui::MenuItem("Medium")) {}
            if (ImGui::MenuItem("Hard")) {}
            if (ImGui::MenuItem("Custom")) {}

            // End the "Difficulty" menubar menu
            ImGui::EndMenu();
        }

        // End the menubar
        ImGui::EndMainMenuBar();
    }
}

C_END