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
#include <cstdlib> // For rand()

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

    // Merge Icons into Header Font so we can use them with large text
    io.Fonts->AddFontFromMemoryTTF(
        (void*)FaSolid, sizeof(FaSolid), 13.0f, &iconsConfig, icons_ranges
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


// 1. ADD THIS HELPER: Draws text with a drop shadow
void CenterTextWithShadow(const char* text, ImFont* font, float scale, ImVec4 color, ImVec4 shadowCol = ImVec4(0, 0, 0, 0.8f)) {
    float winWidth = ImGui::GetColumnWidth();

    ImGui::PushFont(font);
    ImGui::SetWindowFontScale(scale);

    ImVec2 textSize = ImGui::CalcTextSize(text);
    float cursorX = ImGui::GetCursorPosX() + (winWidth - textSize.x) / 2.0f;
    float cursorY = ImGui::GetCursorPosY();

    // Draw Shadow
    ImGui::SetCursorPos(ImVec2(cursorX + 2.0f, cursorY + 2.0f)); // 2px Offset
    ImGui::PushStyleColor(ImGuiCol_Text, shadowCol);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();

    // Draw Main Text
    ImGui::SetCursorPos(ImVec2(cursorX, cursorY));
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("%s", text);
    ImGui::PopStyleColor();

    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopFont();
}

// 2. ADD THIS HELPER: Draws the Icon with a shadow
void CenterIconWithShadow(const char* icon, float scale, ImVec4 color) {
    float winWidth = ImGui::GetColumnWidth();

    // Calculate size with scale
    ImGui::SetWindowFontScale(scale);
    ImVec2 iconSize = ImGui::CalcTextSize(icon);
    ImGui::SetWindowFontScale(1.0f); // Reset immediately so we don't mess up other things

    float cursorX = ImGui::GetCursorScreenPos().x + (winWidth - iconSize.x) / 2.0f;
    float cursorY = ImGui::GetCursorScreenPos().y;

    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Draw Shadow (Offset +2px)
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale, ImVec2(cursorX + 2.0f, cursorY + 2.0f), IM_COL32(0, 0, 0, 180), icon);
    // Draw Icon
    draw->AddText(ImGui::GetFont(), ImGui::GetFontSize() * scale, ImVec2(cursorX, cursorY), ImGui::ColorConvertFloat4ToU32(color), icon);

    // Dummy to advance layout cursor
    ImGui::Dummy(ImVec2(iconSize.x, iconSize.y));
}

void GUIManager::RenderStatsHUD(float scale)
{
    ImGuiIO& io = ImGui::GetIO();
    float dt = io.DeltaTime;

    // =========================================================
    // 1. DEBUG SIMULATION
    // =========================================================

    // K = Damage, L = Heal, M = Score
    if (ImGui::IsKeyPressed(ImGuiKey_K)) playerHealth -= 15;
    if (ImGui::IsKeyPressed(ImGuiKey_L)) playerHealth += 15;
    if (ImGui::IsKeyPressed(ImGuiKey_M)) playerScore += 500;

    // --- NEW MECHANIC SIMULATION ---

    // 'J' = Collect Trash (Fill Backpack)
    if (ImGui::IsKeyPressed(ImGuiKey_J))
    {
        backpackCount++;
        backpackPulseTimer = 1.0f; // Pulse Gold

        // CRAFTING EVENT (Loop Complete)
        if (backpackCount >= maxBackpack)
        {
            backpackCount = 0;      // Reset Backpack
            antidoteCount++;        // Gain Ammo

            // Trigger "POP" Animation
            antidoteScaleTimer = 1.0f;
            antidoteScaleDir = 1.0f; // Grow
        }
    }

    // 'O' = Shoot Antidote
    if (ImGui::IsKeyPressed(ImGuiKey_O))
    {
        if (antidoteCount > 0) {
            antidoteCount--;

            // Trigger "RECOIL" Animation
            antidoteScaleTimer = 0.5f; // Faster than grow
            antidoteScaleDir = -1.0f;  // Shrink
        }
    }

    // Safety Clamps
    if (playerHealth < 0) playerHealth = 0;
    if (playerHealth > 100) playerHealth = 100;

    // =========================================================
    // 2. ANIMATION UPDATES
    // =========================================================

    // Health Logic (Same as before)
    if (lastHealth == -1) { lastHealth = playerHealth; displayedHealth = (float)playerHealth; }
    if (playerHealth != lastHealth) {
        healthFlashTimer = 1.0f;
        if (playerHealth < lastHealth) { healthChangeDir = -1; healthShakeTimer = 0.5f; }
        else { healthChangeDir = 1; healthShakeTimer = 0.0f; }
        lastHealth = playerHealth;
    }
    if (healthFlashTimer > 0.0f) healthFlashTimer -= dt * 3.0f; else healthFlashTimer = 0.0f;
    if (healthShakeTimer > 0.0f) healthShakeTimer -= dt; else healthShakeTimer = 0.0f;

    // Health Number Trap Door
    if (playerHealth < displayedHealth) displayedHealth = (float)playerHealth;
    else if (playerHealth > displayedHealth) {
        if (std::abs(displayedHealth - (float)playerHealth) < 0.5f) displayedHealth = (float)playerHealth;
        else displayedHealth += ((float)playerHealth - displayedHealth) * 20.0f * dt * 0.1f;
    }

    // Score Logic
    if (std::abs(displayedScore - (float)playerScore) > 1.0f)
        displayedScore += ((float)playerScore - displayedScore) * 5.0f * dt;
    else displayedScore = (float)playerScore;

    // NEW: Antidote/Backpack Timers
    if (antidoteScaleTimer > 0.0f) antidoteScaleTimer -= dt * 4.0f; // Fast Pop/Recoil recovery
    else antidoteScaleTimer = 0.0f;

    if (backpackPulseTimer > 0.0f) backpackPulseTimer -= dt * 3.0f;
    else backpackPulseTimer = 0.0f;


    // =========================================================
    // 3. RENDERING
    // =========================================================

    ImU32 bgCol = IM_COL32(15, 15, 20, 220);
    ImU32 borderCol = IM_COL32(255, 255, 255, 30);
    ImVec4 colGold = ImVec4(0.78f, 0.70f, 0.39f, 1.0f);
    ImVec4 colRed = ImVec4(0.9f, 0.2f, 0.2f, 1.0f);
    ImVec4 colGreen = ImVec4(0.2f, 0.9f, 0.2f, 1.0f);
    ImVec4 colWhite = ImVec4(1.0f, 1.0f, 1.0f, 0.9f);
    ImVec4 colGrey = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    ImVec4 colDim = ImVec4(0.4f, 0.4f, 0.4f, 1.0f); // For empty ammo
    ImU32 sepCol = IM_COL32(255, 255, 255, 15);

    float width = 480.0f * scale;
    float height = 65.0f * scale;
    float rounding = 4.0f * scale;
    float bottomMargin = 15.0f * scale;
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Shake Logic
    float shakeX = 0.0f; float shakeY = 0.0f;
    if (healthShakeTimer > 0.0f) {
        float intensity = (5.0f * scale) * (healthShakeTimer / 0.5f);
        shakeX = (float)((rand() % 100) - 50) / 50.0f * intensity;
        shakeY = (float)((rand() % 100) - 50) / 50.0f * intensity;
    }

    ImGui::SetNextWindowPos(ImVec2((viewport->Size.x - width) / 2.0f + shakeX, viewport->Size.y - height - bottomMargin + shakeY));
    ImGui::SetNextWindowSize(ImVec2(width, height));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("StatsHUD", nullptr, flags))
    {

        ImGui::SetWindowFontScale(scale);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();

        draw->AddRectFilled(p, ImVec2(p.x + width, p.y + height), bgCol, rounding);
        draw->AddRect(p, ImVec2(p.x + width, p.y + height), borderCol, rounding);

        if (ImGui::BeginTable("StatsTable", 3, ImGuiTableFlags_SizingStretchSame))
        {
            // --- SHARED HELPER FOR HEALTH & SCORE ---
            auto DrawSimpleBlock = [&](const char* label, const char* value, const char* icon, ImU32 iconColor, ImU32 textColor, bool showSeparator)
                {
                    ImGui::TableNextColumn();
                    float colWidth = ImGui::GetColumnWidth();
                    float startX = ImGui::GetCursorScreenPos().x;
                    float centerY = ImGui::GetCursorScreenPos().y + (height / 2.0f);

                    ImGui::PushFont(fontMono); ImVec2 iconSz = ImGui::CalcTextSize(icon); ImGui::PopFont();
                    ImGui::PushFont(fontMono); ImVec2 labelSz = ImGui::CalcTextSize(label); ImGui::PopFont();
                    ImGui::PushFont(fontHeader); ImVec2 valueSz = ImGui::CalcTextSize(value); ImGui::PopFont();

                    float gap = 22.0f * scale;
                    float iconOffset = -gap - iconSz.x;
                    float valueOffset = (labelSz.x - valueSz.x) / 2.0f; // Center value under label

                    float groupMinX = (iconOffset < valueOffset) ? iconOffset : valueOffset;
                    float groupMaxX = (labelSz.x > (valueOffset + valueSz.x)) ? labelSz.x : (valueOffset + valueSz.x);
                    float totalGroupWidth = groupMaxX - groupMinX;

                    float colCenter = startX + (colWidth / 2.0f);
                    float screenLabelStart = colCenter - (totalGroupWidth / 2.0f) - groupMinX;

                    ImGui::PushFont(fontMono);
                    draw->AddText(ImVec2(screenLabelStart + iconOffset, centerY - iconSz.y / 2.0f), iconColor, icon);
                    ImGui::PopFont();

                    ImGui::PushFont(fontMono);
                    draw->AddText(ImVec2(screenLabelStart, centerY - labelSz.y - (1.0f * scale)), ImGui::ColorConvertFloat4ToU32(colGrey), label);
                    ImGui::PopFont();

                    ImGui::PushFont(fontHeader);
                    draw->AddText(ImVec2(screenLabelStart + valueOffset, centerY + (1.0f * scale)), textColor, value);
                    ImGui::PopFont();

                    if (showSeparator) {
                        float sepH = height * 0.5f;
                        float sepY = p.y + (height - sepH) / 2.0f;
                        draw->AddLine(ImVec2(startX + colWidth, sepY), ImVec2(startX + colWidth, sepY + sepH), sepCol, 1.0f * scale);
                    }
                };

            // 1. HEALTH (Trap Door Anim)
            {
                ImVec4 cIcon = colGold; ImVec4 cText = colWhite;
                if (healthFlashTimer > 0.0f) {
                    ImVec4 tCol = (healthChangeDir == -1) ? colRed : colGreen;
                    float t = healthFlashTimer;
                    cIcon = ImVec4(cIcon.x + (tCol.x - cIcon.x) * t, cIcon.y + (tCol.y - cIcon.y) * t, cIcon.z + (tCol.z - cIcon.z) * t, 1);
                    cText = ImVec4(cText.x + (tCol.x - cText.x) * t, cText.y + (tCol.y - cText.y) * t, cText.z + (tCol.z - cText.z) * t, 1);
                }
                //DrawSimpleBlock("HEALTH", std::to_string((int)displayedHealth).c_str(), ICON_FA_HEART, ImGui::ColorConvertFloat4ToU32(cIcon), ImGui::ColorConvertFloat4ToU32(cText), true);
                char healthBuf[16];
                sprintf_s(healthBuf, "%d", (int)displayedHealth);
                DrawSimpleBlock("HEALTH", healthBuf, ICON_FA_HEART, ImGui::ColorConvertFloat4ToU32(cIcon), ImGui::ColorConvertFloat4ToU32(cText), true);
            }

            // 2. SCORE (Rolling Anim)
            {
                char sb[32]; int ds = (int)displayedScore;
                if (ds >= 1000) sprintf_s(sb, "%.1fk", (float)ds / 1000.0f); else sprintf_s(sb, "%d", ds);
                DrawSimpleBlock("SCORE", sb, ICON_FA_TROPHY, ImGui::ColorConvertFloat4ToU32(colGold), ImGui::ColorConvertFloat4ToU32(colWhite), true);
            }

            // --- 3. ANTIDOTE & BACKPACK (CUSTOM RENDERER) ---
            ImGui::TableNextColumn();
            {
                float colWidth = ImGui::GetColumnWidth();
                float startX = ImGui::GetCursorScreenPos().x;
                float centerY = ImGui::GetCursorScreenPos().y + (height / 2.0f);

                const char* icon = ICON_FA_SYRINGE;
                const char* label = "ANTIDOTE";

                // Format Values
                char antVal[16];
                sprintf_s(antVal, "%d", antidoteCount);
                char packVal[32]; sprintf_s(packVal, "%d/%d", backpackCount, maxBackpack);

                // --- 1. CALCULATE WIDTHS ---
                ImGui::PushFont(fontMono);   ImVec2 iconSz = ImGui::CalcTextSize(icon);    ImGui::PopFont();
                ImGui::PushFont(fontMono);   ImVec2 labelSz = ImGui::CalcTextSize(label);   ImGui::PopFont();

                // Scale Logic for Big Number
                float currentScale = 1.0f;
                if (antidoteScaleTimer > 0.0f) {
                    // Easing: (Pop=1.5, Shrink=0.8) -> Return to 1.0
                    float target = (antidoteScaleDir > 0) ? 1.5f : 0.8f;
                    currentScale = 1.0f + (target - 1.0f) * antidoteScaleTimer; // Simple Linear return
                }

                ImGui::PushFont(fontHeader);
                ImGui::SetWindowFontScale(currentScale * scale); // <--- Combine scales
                ImVec2 antSz = ImGui::CalcTextSize(antVal);
                ImGui::SetWindowFontScale(scale);         // Reset to global scale
                ImGui::PopFont();

                ImGui::PushFont(fontUI);
                ImVec2 packSz = ImGui::CalcTextSize(packVal);
                ImGui::PopFont();

                float valGap = 8.0f * scale; // Scale gap
                float compositeValueWidth = antSz.x + valGap + packSz.x;

                float gap = 22.0f * scale; // Scale gap
                float iconOffset = -gap - iconSz.x;

                // Center the COMPOSITE VALUE block under the Label
                float valueGroupOffset = (labelSz.x - compositeValueWidth) / 2.0f;

                float groupMinX = (iconOffset < valueGroupOffset) ? iconOffset : valueGroupOffset;
                float groupMaxX = (labelSz.x > (valueGroupOffset + compositeValueWidth)) ? labelSz.x : (valueGroupOffset + compositeValueWidth);
                float totalGroupWidth = groupMaxX - groupMinX;

                float colCenter = startX + (colWidth / 2.0f);
                float screenLabelStart = colCenter - (totalGroupWidth / 2.0f) - groupMinX;

                // --- 4. DRAW ---

                // Draw Icon
                ImGui::PushFont(fontMono);
                ImU32 iconColor = (antidoteCount == 0) ? ImGui::ColorConvertFloat4ToU32(colDim) : ImGui::ColorConvertFloat4ToU32(colGold);
                draw->AddText(ImVec2(screenLabelStart + iconOffset, centerY - iconSz.y / 2.0f), iconColor, icon);
                ImGui::PopFont();

                // Draw Label
                ImGui::PushFont(fontMono);
                draw->AddText(ImVec2(screenLabelStart, centerY - labelSz.y - (1.0f * scale)), ImGui::ColorConvertFloat4ToU32(colGrey), label);
                ImGui::PopFont();

                // Draw Composite Value
                float currentX = screenLabelStart + valueGroupOffset;
                float baselineY = centerY + (1.0f * scale);

                // A. Big Antidote Number (Centered vertically based on its scaled height)
                ImGui::PushFont(fontHeader);
                ImGui::SetWindowFontScale(currentScale* scale);
                ImU32 antColor = (antidoteCount == 0) ? ImGui::ColorConvertFloat4ToU32(colRed) : ImGui::ColorConvertFloat4ToU32(colWhite);

                // Adjustment to keep text centered when scaling
                float scaleShiftX = (antSz.x - (antSz.x / currentScale)) / 2.0f;
                float scaleShiftY = (antSz.y - (antSz.y / currentScale)) / 2.0f;

                draw->AddText(ImVec2(currentX - scaleShiftX, baselineY - scaleShiftY), antColor, antVal);

                ImGui::SetWindowFontScale(scale); // Reset
                ImGui::PopFont();

                // Move cursor past big number
                currentX += antSz.x + valGap;

                // B. Small Backpack Number
                ImGui::PushFont(fontUI);
                ImU32 packColor = ImGui::ColorConvertFloat4ToU32(colGrey);

                // Flash Gold if collecting
                if (backpackPulseTimer > 0.0f) {
                    ImVec4 cPack = colGrey;
                    float t = backpackPulseTimer;
                    cPack = ImVec4(cPack.x + (colGold.x - cPack.x) * t, cPack.y + (colGold.y - cPack.y) * t, cPack.z + (colGold.z - cPack.z) * t, 1);
                    packColor = ImGui::ColorConvertFloat4ToU32(cPack);
                }

                // Vertical align small text to the baseline of big text roughly
                // Or just center relative to centerY
                draw->AddText(ImVec2(currentX, baselineY + (2.0f * scale)), packColor, packVal);
                ImGui::PopFont();
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}



void GUIManager::Render(const glm::vec3& playerPos, int screenWidth, int screenHeight, float fps, int renderedObjects, int currentHealth, int currentBackpack, int currentAntidotes)
{
    //if (!showGUI) return;
    this->playerHealth = currentHealth;
    this->backpackCount = currentBackpack;
    this->antidoteCount = currentAntidotes;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // --- 1. CALCULATE SCALE FACTOR ---
   // Reference height is 1080.0f.
   // If screen is 2160p (4K), scale becomes 2.0f.
    float uiScale = (float)screenHeight / 1080.0f;

    // Safety clamp (don't let it get too small on weird windows)
    if (uiScale < 0.5f) uiScale = 0.5f;


    questManager.Render(fontUI, fontHeader, fontMono, uiScale);

    RenderStatsHUD(uiScale);

    if (showGUI)
    {
        // Styling Setup of Window
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.24f, 0.24f, 1.0f));

        // Header Colors (for Collapsing Headers)
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.15f, 0.15f, 1.0f)); // Idle
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.20f, 0.20f, 1.0f)); // Hover
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.18f, 0.18f, 1.0f)); // Click

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10 * uiScale, 10 * uiScale));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f * uiScale);

        // Setup Window
        ImGui::SetNextWindowSize(ImVec2(320 * uiScale, 500 * uiScale), ImGuiCond_FirstUseEver);
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings;

        if (ImGui::Begin("GamePanel", nullptr, window_flags))
        {
            ImGui::SetWindowFontScale(uiScale);

            // HEADER SECTION
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 p = ImGui::GetCursorScreenPos();

                // Draw the "Blue Dot". We manually draw this because it glows in the design
                drawList->AddCircleFilled(ImVec2(p.x + (4 * uiScale), p.y + (9 * uiScale)), 4.0f * uiScale, IM_COL32(59, 130, 246, 255));

                ImGui::SetCursorPosX(25 * uiScale);

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
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 6.0f * uiScale));
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
                    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 150.0f * uiScale);

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