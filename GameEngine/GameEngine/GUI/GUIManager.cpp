#include "GUIManager.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#include <string>
#include "imgui_internal.h"

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

    // Add some initial logs
    AddLog("[SYSTEM] Engine initialized");
    AddLog("[RENDER] OpenGL context ready");
}
bool test = false;

void DrawToggleSwitch(const char* label, bool* v)
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

    // Invisible button handles the click logic
    if (ImGui::InvisibleButton("##toggle_btn", ImVec2(width, height))) {
        *v = !*v;
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
    if (!showGUI) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Style
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1176f, 0.1176f, 0.1176f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.133, 0.133, 0.133, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.239215686f, 0.239215686f, 0.239215686f, 1.0f));

    ImGui::SetNextWindowSize(ImVec2(320, 480), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

    if (ImGui::Begin("Game Control Panel", nullptr, window_flags))
    {
        // CUSTOM HEADER
        {
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            drawList->AddCircleFilled(ImVec2(p.x + 4, p.y + 7), 5.0f, IM_COL32(50, 100, 255, 255));

            ImGui::SetCursorPosX(25);
            ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
            ImGui::Text("Game Control Panel");
            ImGui::PopFont();
            ImGui::Separator();
            ImGui::Spacing();
        }

        // SCENE CONTROLS
        if (ImGui::CollapsingHeader("SCENE CONTROLS", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
            if (ImGui::Button("Change Background", ImVec2(-1, 0))) {
                changeBackground = !changeBackground;
                AddLog("[SYSTEM] Background toggled");
            }
            ImGui::PopStyleColor();
            ImGui::Spacing();

            DrawToggleSwitch("Day / Night Cycle", &dayNightCycle);
            DrawToggleSwitch("Enable Animation", &enableAnimation);
            ImGui::Spacing();
        }

        // DEBUG INFORMATION
        if (ImGui::CollapsingHeader("DEBUG INFORMATION", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Spacing();
            if (ImGui::BeginTable("DebugTable", 2))
            {
                ImGui::TableSetupColumn("Label");
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 180.0f);

                // Row 1: FPS
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "FPS");
                ImGui::TableSetColumnIndex(1);
                char fpsBuff[32];
                sprintf_s(fpsBuff, "%.1f", fps);
                TextRightAligned(fpsBuff, ImVec4(0.4f, 1.0f, 0.4f, 1.0f)); // Greenish text

                // Row 2 Player Pos
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Player Pos");
                ImGui::TableSetColumnIndex(1);
                char posBuffer[64];
                sprintf_s(posBuffer, "X:%.1f Y:%.1f Z:%.1f", playerPos.x, playerPos.y, playerPos.z);
                TextRightAligned(posBuffer);

                // Row 3 Resolution
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Resolution");
                ImGui::TableSetColumnIndex(1);
                char resBuffer[32];
                sprintf_s(resBuffer, "%dx%d", screenWidth, screenHeight);
                TextRightAligned(resBuffer);

                // Row 4 Rendered Objects
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Rendered Objects");
                ImGui::TableSetColumnIndex(1);
                char drawBuff[32];
                sprintf_s(drawBuff, "%d", renderedObjects);
                TextRightAligned(drawBuff);

                ImGui::EndTable();
            }
            ImGui::Spacing();
        }

        // LOG OUTPUT
        if (ImGui::CollapsingHeader("LOG OUTPUT", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.08f, 0.08f, 0.08f, 1.0f));
            if (ImGui::BeginChild("LogOutput", ImVec2(0, 100), true))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                for (const auto& log : logs) {
                    ImGui::TextUnformatted(log.c_str());
                }

                // Auto scroll to bottom
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);

                ImGui::PopStyleColor();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        // FOOTER
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImGui::Text("Press 'INSERT' to toggle");

        // Calculate Memory Usage with Windows API
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
    }

    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(4);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void GUIManager::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}