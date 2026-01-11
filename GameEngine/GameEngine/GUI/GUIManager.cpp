#include "GUIManager.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h"
#include <stdio.h>
#include <string>
#include "Fonts/FaSolid.h"
#include "Fonts/IconsFontAwesome7.h"
#include "Fonts/InterBold.h"
#include "Fonts/InterMedium.h"
#include "Fonts/JetBrains.h"

// Windows specific headers for Memory Usage
#include <windows.h>
#include <psapi.h>

void GUIManager::Init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");


    // Config for Text
    ImFontConfig textConfig;
    textConfig.FontDataOwnedByAtlas = false; // For HxD arrays so we don't free memory
    textConfig.OversampleH = 2;
    textConfig.OversampleV = 1;
    textConfig.PixelSnapH = true;

    // Config for Icons
    ImFontConfig iconsConfig;
	iconsConfig.FontDataOwnedByAtlas = false; // For HxD arrays so we don't free memory
    iconsConfig.MergeMode = true;             // Combine with the previous font
    iconsConfig.PixelSnapH = true;
    iconsConfig.GlyphMinAdvanceX = 13.0f;     // Force constant width for icons
    iconsConfig.GlyphOffset = ImVec2(0, 0);

    // Define the range of icons to load (Min to Max)
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

    // Load Default UI Font (Inter Medium 15px)
    fontUI = io.Fonts->AddFontFromMemoryTTF(
        (void*)InterMedium, sizeof(InterMedium), 15.0f, &textConfig
    );

    // Merge Icons into UI Font
    io.Fonts->AddFontFromMemoryTTF(
        (void*)FaSolid, sizeof(FaSolid), 13.0f, &iconsConfig, icons_ranges
    );

    // Load Header Font (Inter Bold 17px)
    fontHeader = io.Fonts->AddFontFromMemoryTTF(
        (void*)InterBold, sizeof(InterBold), 17.0f, &textConfig
    );

    // Load Small Font (Inter Medium 14px)
    fontSmall = io.Fonts->AddFontFromMemoryTTF(
        (void*)InterMedium, sizeof(InterMedium), 14.0f, &textConfig
    );

    // Load Mono Font (JetBrains 15px)
    fontMono = io.Fonts->AddFontFromMemoryTTF(
        (void*)JetBrains, sizeof(JetBrains), 15.0f, &textConfig
    );

    // Merge Icons into Mono Font (So we can use icons in Debug Info)
    io.Fonts->AddFontFromMemoryTTF(
        (void*)FaSolid, sizeof(FaSolid), 13.0f, &iconsConfig, icons_ranges
    );

    io.Fonts->Build();
    io.FontDefault = fontUI;

    questManager.Init();

    // Add some initial logs
    AddLog("[SYSTEM] Engine initialized");
    AddLog("[RENDER] OpenGL context ready");
}
bool test = false;

bool DrawToggleSwitch(const char* label, bool* v)
{
    // Text on Left
    ImGui::Text("%s", label);
    ImGui::SameLine();

    float height = ImGui::GetFrameHeight();
    float width = height * 1.55f;
    float radius = height * 0.50f;

    // Push to the right side
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - width);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    // We use ImGui storage to save the animation progress (0.0f to 1.0f) for this specific button ID
    ImGui::PushID(label);
    ImGuiID id = ImGui::GetID("##toggle_state");
    ImGuiStorage* storage = ImGui::GetStateStorage();

    // Retrieve previous animation state
    float t = storage->GetFloat(id, *v ? 1.0f : 0.0f);

    float dt = ImGui::GetIO().DeltaTime;
    float speed = 8.0f; // Animation speed
    if (*v) {
        t += dt * speed; // Animate towards 1.0
    }
    else {
        t -= dt * speed; // Animate towards 0.0
    }
    t = ImClamp(t, 0.0f, 1.0f); // Clamp between 0 and 1
    storage->SetFloat(id, t);   // Save state for next frame

    bool pressed = false;
    if (ImGui::InvisibleButton("##toggle_btn", ImVec2(width, height))) {
        *v = !*v;
        pressed = true; // Mark as clicked
    }


    ImVec4 col_off = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    ImVec4 col_on = ImColor(IM_COL32(50, 100, 255, 255));//ImVec4(0.19f, 0.8f, 0.19f, 1.0f);

    ImVec4 bg_color_vec;
    bg_color_vec.x = col_off.x + (col_on.x - col_off.x) * t;
    bg_color_vec.y = col_off.y + (col_on.y - col_off.y) * t;
    bg_color_vec.z = col_off.z + (col_on.z - col_off.z) * t;
    bg_color_vec.w = 1.0f;

    draw_list->AddRectFilled(p, ImVec2(p.x + width, p.y + height), ImGui::ColorConvertFloat4ToU32(bg_color_vec), height * 0.5f);

    float pad = 2.0f;
    float circle_radius = radius - pad;

    float circle_x_start = p.x + radius;
    float circle_x_end = p.x + width - radius;
    float circle_x = circle_x_start + (circle_x_end - circle_x_start) * t;

    draw_list->AddCircleFilled(ImVec2(circle_x, p.y + radius + 1.5f), circle_radius, IM_COL32(0, 0, 0, 50));

    draw_list->AddCircleFilled(ImVec2(circle_x, p.y + radius), circle_radius, IM_COL32(255, 255, 255, 255));

    ImGui::PopID();
    return pressed;
}

// Helper to align text to right
void TextRightAligned(const char* text, ImVec4 color = ImVec4(1, 1, 1, 1)) {
    float width = ImGui::CalcTextSize(text).x;
    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - width);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();
}

void GUIManager::Render(const glm::vec3& playerPos, int screenWidth, int screenHeight, float fps, int renderedObjects)
{
    //if (!showGUI) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    questManager.Render(fontUI, fontHeader, fontMono);
    if (showGUI)
    {
        // Styling Setup of Window
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.24f, 0.24f, 1.0f));

        // Header Colors (for Collapsing Headers)
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.15f, 0.15f, 1.0f)); // Idle
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f)); // Hover
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.18f, 0.18f, 1.0f)); // Click

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f); // Slight rounding on elements

        // Setup Window
        ImGui::SetNextWindowSize(ImVec2(320, 500), ImGuiCond_FirstUseEver);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("GamePanel", nullptr, window_flags))
        {
            // HEADER SECTION
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetCursorScreenPos();

                // Draw the "Blue Dot". We manually draw this because it glows in the design
                drawList->AddCircleFilled(ImVec2(p.x + 4, p.y + 9), 4.0f, IM_COL32(59, 130, 246, 255)); // Blue

                // Text Offset
                ImGui::SetCursorPosX(25);

                // Big Bold Title
                ImGui::PushFont(fontHeader);
                ImGui::Text("Game Control Panel");
                ImGui::PopFont();

                // Version (Right Aligned, Mono Font)
                ImGui::SameLine();
                ImGui::PushFont(fontMono);
                TextRightAligned("v1.0", ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::PopFont();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
            }

            // SCENE CONTROLS SECTION
            ImGui::PushFont(fontSmall); // Small Caps Header

            if (ImGui::CollapsingHeader("SCENE CONTROLS", ImGuiTreeNodeFlags_DefaultOpen)) // DefaultOpen means it starts expanded
            {
                ImGui::PopFont(); // Switch back to Default UI Font (Inter Medium)
                ImGui::Spacing();

                // "Change Background" Button
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 6.0f));
                if (ImGui::Button(ICON_FA_LAYER_GROUP "  Change Background", ImVec2(-1, 0))) {
                    changeBackground = !changeBackground;
                    AddLog("[SYSTEM] Background toggled");
                }
                ImGui::PopStyleVar();
                ImGui::PopStyleColor();

                ImGui::Spacing();

                // Custom Toggles (Inter Medium)
                if (DrawToggleSwitch("Day / Night Cycle", &dayNightCycle)) {
                    AddLog(dayNightCycle ? "[SCENE] Changed to Night Scene" : "[SCENE] Changed to Day Scene");
                }
                ImGui::Spacing();
                DrawToggleSwitch("Enable Animation", &enableAnimation);

                /*
                if (DrawToggleSwitch("Enable Animation", &enableAnimation)) {
                     AddLog(enableAnimation ? "[ANIM] Animation started" : "[ANIM] Animation stopped");
                 }*/
                ImGui::Spacing();
            }
            else
            {
                ImGui::PopFont(); // Pop if closed
            }

            // DEBUG INFORMATION SECTION
            ImGui::PushFont(fontSmall);
            if (ImGui::CollapsingHeader("DEBUG INFORMATION", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PopFont();
                ImGui::Spacing();

                ImGui::PushFont(fontMono);

                if (ImGui::BeginTable("DebugTable", 2))
                {
                    ImGui::TableSetupColumn("Label");
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 150.0f);

                    // Row 1 FPS
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(ICON_FA_WAVE_SQUARE "  FPS");
                    ImGui::TableSetColumnIndex(1);
                    char fpsBuff[32];
                    sprintf_s(fpsBuff, "%.0f", fps);
                    // Highlight FPS in Blue
                    TextRightAligned(fpsBuff, ImVec4(0.29f, 0.62f, 1.0f, 1.0f));

                    // Row 2 Position
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(ICON_FA_ARROW_POINTER "  Player Pos");
                    ImGui::TableSetColumnIndex(1);
                    char posBuffer[64];
                    sprintf_s(posBuffer, "%.1f, %.1f, %.1f", playerPos.x, playerPos.y, playerPos.z);
                    TextRightAligned(posBuffer);

                    // Row 3 Resolution
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(ICON_FA_DESKTOP "  Resolution");
                    ImGui::TableSetColumnIndex(1);
                    char resBuffer[32];
                    sprintf_s(resBuffer, "%dx%d", screenWidth, screenHeight);
                    TextRightAligned(resBuffer);

                    // Row 4 Objects
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text(ICON_FA_GAMEPAD "  Objects");
                    ImGui::TableSetColumnIndex(1);
                    char objBuff[32];
                    sprintf_s(objBuff, "%d", renderedObjects);
                    TextRightAligned(objBuff);

                    ImGui::EndTable();
                }
                ImGui::PopFont(); // Stop using Mono
                ImGui::Spacing();
            }
            else
            {
                ImGui::PopFont();
            }

            // LOG OUTPUT
            ImGui::PushFont(fontSmall);
            if (ImGui::CollapsingHeader("LOG OUTPUT", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PopFont();

                // Darker background for logs
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.07f, 1.0f));
                if (ImGui::BeginChild("LogOutput", ImVec2(0, 100), true))
                {
                    ImGui::PushFont(fontMono); // Logs look better in Mono
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));

                    for (const auto& log : logs) {
                        ImGui::TextUnformatted(log.c_str());
                    }

                    // Auto Scroll
                    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                        ImGui::SetScrollHereY(1.0f);

                    ImGui::PopStyleColor();
                    ImGui::PopFont();
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
            }
            else
            {
                ImGui::PopFont();
            }

            // FOOTER
            ImGui::Spacing();
            ImGui::Separator();

            // Footer in Mono
            ImGui::PushFont(fontMono);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

            ImGui::Text("Press 'INSERT' to toggle");

            // Memory
            PROCESS_MEMORY_COUNTERS pmc;
            float memUsageMB = 0.0f;
            if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
                memUsageMB = pmc.WorkingSetSize / 1024.0f / 1024.0f;
            }
            char memBuff[32];
            sprintf_s(memBuff, "Mem: %.1f MB", memUsageMB);
            ImGui::SameLine();
            TextRightAligned(memBuff);

            ImGui::PopStyleColor();
            ImGui::PopFont();
        }

        ImGui::End();

        // Cleanup Styles
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUIManager::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}