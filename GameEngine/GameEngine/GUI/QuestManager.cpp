#include "QuestManager.h"
#include "imgui.h"             // Added to fix ImFont incomplete type errors
#include "imgui_internal.h"    // Added for internal math if needed
#include <iostream>
#include <cmath> 
#include <algorithm> 
#include "Fonts/IconsFontAwesome7.h" 

void QuestManager::Init() {
    // --- THE 5 STORY TASKS ---
    AddQuest("Locate the Signal", "The distress beacon is coming from the North Ridge.");
    AddQuest("Power the Generator", "Find 3 Fuel Cells scattered in the ruins.");
    AddQuest("Decrypt Data Terminal", "The password is hidden in the Captain's log.");
    AddQuest("Open the Bunker Door", "Use the terminal to unlock the main gate.");
    AddQuest("Secure the Artifact", "Retrieve the object and return to the extraction point.");

    if (!quests.empty()) {
        currentQuestIndex = 0;
        quests[0].isActive = true;
        isAnnouncementActive = true;
    }
}

void QuestManager::AddQuest(std::string title, std::string desc) {
    QuestTask q;
    q.title = title;
    q.description = desc;
    quests.push_back(q);
}

void QuestManager::CompleteCurrentQuest() {
    // Safety check: Prevent spamming past the end
    if (currentQuestIndex >= 0 && currentQuestIndex < quests.size()) {
        quests[currentQuestIndex].isCompleted = true;
        quests[currentQuestIndex].isActive = false;
        currentQuestIndex++;

        // Only announce if there is actually a next quest
        if (currentQuestIndex < quests.size()) {
            quests[currentQuestIndex].isActive = true;
            isAnnouncementActive = true;
            announcementTimer = 0.0f;
        }
        else {
            // All done - ensure announcement is off so we don't crash trying to render a non-existent title
            isAnnouncementActive = false;
        }
    }
}

// Added 'float scale' to arguments
void QuestManager::Render(ImFont* fontUI, ImFont* fontHeader, ImFont* fontMono, float scale, bool showDetails) {
    if (quests.empty()) return;

    float dt = ImGui::GetIO().DeltaTime;
    pulseTimer += dt * 3.0f;

    if (isAnnouncementActive) {
        announcementTimer += dt;
        if (announcementTimer > 5.5f) {
            isAnnouncementActive = false;
        }
    }

    // -----------------------------------------------------------
    // PART A: CINEMATIC CENTER BANNER
    // -----------------------------------------------------------
    if (isAnnouncementActive) {

        // CRASH FIX - If game finished stop rendering the banner immediately
        if (currentQuestIndex >= quests.size()) {
            isAnnouncementActive = false;
            return;
        }
        float expandProgress = std::min(announcementTimer / 0.8f, 1.0f);
        float widthExpand = 1.0f - pow(1.0f - expandProgress, 3.0f);

        float alpha = 0.0f;
        if (announcementTimer > 0.2f && announcementTimer < 4.5f) {
            alpha = std::min((announcementTimer - 0.2f) / 0.8f, 1.0f);
        }
        else if (announcementTimer >= 4.5f) {
            alpha = 1.0f - ((announcementTimer - 4.5f) / 1.0f);
        }

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        ImVec2 center = viewport->GetCenter();

        ImU32 colTextHeader = IM_COL32(255, 255, 255, 255 * alpha);
        ImU32 colTextLabel = IM_COL32(180, 180, 190, 220 * alpha);
        ImU32 colLineCenter = IM_COL32(200, 180, 100, (int)(255 * alpha)); //IM_COL32(255, 255, 255, (int)(255 * alpha));
        ImU32 colLineEdge = IM_COL32(255, 255, 255, 0);
        ImU32 colBgCenter = IM_COL32(0, 0, 0, 180 * alpha);
        ImU32 colBgEdge = IM_COL32(0, 0, 0, 0);

        // Apply Scale
        float maxLineWidth = 500.0f * scale;
        float currentLineWidth = maxLineWidth * widthExpand;
        float bgHeight = 100.0f * scale;

        drawList->AddRectFilledMultiColor(
            ImVec2(center.x - currentLineWidth * 1.2f, center.y - bgHeight / 2),
            ImVec2(center.x, center.y + bgHeight / 2),
            colBgEdge, colBgCenter, colBgCenter, colBgEdge
        );
        drawList->AddRectFilledMultiColor(
            ImVec2(center.x, center.y - bgHeight / 2),
            ImVec2(center.x + currentLineWidth * 1.2f, center.y + bgHeight / 2),
            colBgCenter, colBgEdge, colBgEdge, colBgCenter
        );

        float lineY = center.y;
        float lineThick = 2.0f * scale;

        drawList->AddRectFilledMultiColor(
            ImVec2(center.x - currentLineWidth, lineY),
            ImVec2(center.x, lineY + lineThick),
            colLineEdge, colLineCenter, colLineCenter, colLineEdge
        );
        drawList->AddRectFilledMultiColor(
            ImVec2(center.x, lineY),
            ImVec2(center.x + currentLineWidth, lineY + lineThick),
            colLineCenter, colLineEdge, colLineEdge, colLineCenter
        );

        // -- Text Drawing with Manual Scaling (Fixes ImFont::FontSize error) --

        const char* label = "NEW OBJECTIVE";
        float labelSizePx = 15.0f * scale; // Hardcoded base size from Init
        ImVec2 labelSize = fontMono->CalcTextSizeA(labelSizePx, FLT_MAX, 0.0f, label);

        drawList->AddText(fontMono, labelSizePx,
            ImVec2(center.x - labelSize.x / 2, lineY - labelSize.y - (15.0f * scale)),
            colTextLabel, label);

        const char* title = quests[currentQuestIndex].title.c_str();
        float titleSizePx = 17.0f * scale; // Hardcoded base size from Init
        ImVec2 titleSize = fontHeader->CalcTextSizeA(titleSizePx, FLT_MAX, 0.0f, title);

        drawList->AddText(fontHeader, titleSizePx,
            ImVec2(center.x - titleSize.x / 2, lineY + (15.0f * scale)),
            colTextHeader, title);

        if (announcementTimer < 4.0f) return;
    }

    // -----------------------------------------------------------
    // DOCKED WIDGET
    // -----------------------------------------------------------

    float hudAlpha = 1.0f;
    if (isAnnouncementActive) {
        if (announcementTimer >= 4.5f) {
            hudAlpha = (announcementTimer - 4.5f) / 1.0f;
        }
        else {
            hudAlpha = 0.0f;
        }
    }

    ImU32 bgCol = IM_COL32(15, 15, 20, 200 * hudAlpha);
    ImU32 borderCol = IM_COL32(255, 255, 255, 30 * hudAlpha);
    ImU32 textCol = IM_COL32(255, 255, 255, 230 * hudAlpha);
    ImU32 accentCol = IM_COL32(200, 180, 100, 255 * hudAlpha);

    // Apply Scale to Dimensions
    float width = 224.0f * scale;
    float baseHeight = 60.0f * scale;
    float rounding = 4.0f * scale;

    // Scale Position
    ImGui::SetNextWindowPos(ImVec2(20 * scale, 40 * scale));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Remove Borders and Padding so window size == drawing size
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("QuestTracker", nullptr, flags))
    {
        // Apply Global Window Font Scale
        ImGui::SetWindowFontScale(scale);

        // Animation Logic
        if (showDetails) hoverAnimation += dt * 5.0f;
        else hoverAnimation -= dt * 5.0f;

        if (hoverAnimation < 0.0f) hoverAnimation = 0.0f;
        if (hoverAnimation > 1.0f) hoverAnimation = 1.0f;

        float historyHeight = 0.0f;
        int completedCount = 0;
        for (const auto& q : quests) if (q.isCompleted) completedCount++;

        // Scale History Height
        if (completedCount > 0) historyHeight = completedCount * (28.0f * scale) + (25.0f * scale);

        float totalHeight = baseHeight + (historyHeight * hoverAnimation);

        // Set Dummy to TOTAL HEIGHT to expand the window so the bottom border isnt clipped
        ImGui::Dummy(ImVec2(width, totalHeight));

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p = ImGui::GetWindowPos();

        // Draw Unified Background
        draw->AddRectFilled(p, ImVec2(p.x + width, p.y + totalHeight), bgCol, rounding);
        draw->AddRect(p, ImVec2(p.x + width, p.y + totalHeight), borderCol, rounding);

        // Draw Icon (Scaled)
        float cx = p.x + (30.0f * scale);
        float cy = p.y + baseHeight / 2.0f;
        float r = 7.0f * scale;

        ImVec2 p1(cx, cy - r); ImVec2 p2(cx + r, cy);
        ImVec2 p3(cx, cy + r); ImVec2 p4(cx - r, cy);
        draw->AddQuadFilled(p1, p2, p3, p4, accentCol);

        // Draw Text

        // Setup Strings
        const char* labelStr = "CURRENT OBJECTIVE";
        const char* titleStr = (currentQuestIndex < quests.size()) ? quests[currentQuestIndex].title.c_str() : "All Objectives Complete";

        // Calculate Heights
        ImGui::PushFont(fontMono);
        float labelHeight = ImGui::CalcTextSize(labelStr).y;
        ImGui::PopFont();

        ImGui::PushFont(fontUI);
        float titleHeight = ImGui::CalcTextSize(titleStr).y;
        ImGui::PopFont();

        // Vertical Math
        float textGap = 4.0f * scale;
        float totalTextHeight = labelHeight + textGap + titleHeight;
        float blockStartY = p.y + (baseHeight - totalTextHeight) / 2.0f;

        // Horizontal Math
        float textStartX = p.x + (30.0f + 5.0f + 17.0f) * scale;

        // Render Text
        ImGui::SetCursorScreenPos(ImVec2(textStartX, blockStartY));
        ImGui::PushFont(fontMono);
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(150, 150, 150, 200 * hudAlpha));
        ImGui::Text("%s", labelStr);
        ImGui::PopStyleColor();
        ImGui::PopFont();

        ImGui::SetCursorScreenPos(ImVec2(textStartX, blockStartY + labelHeight + textGap));
        ImGui::PushFont(fontUI);
        ImGui::PushStyleColor(ImGuiCol_Text, textCol);
        ImGui::Text("%s", titleStr);
        ImGui::PopStyleColor();
        ImGui::PopFont();

        // Draw History
        if (hoverAnimation > 0.01f && completedCount > 0) {
            draw->PushClipRect(ImVec2(p.x, p.y + baseHeight), ImVec2(p.x + width, p.y + totalHeight), true);

            ImGui::SetCursorPos(ImVec2(20 * scale, baseHeight + (10 * scale)));
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, hoverAnimation * hudAlpha);

            ImGui::PushFont(fontMono);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "COMPLETED");
            ImGui::PopFont();

            ImGui::PushFont(fontUI);
            for (const auto& q : quests) {
                if (q.isCompleted) {
                    ImGui::SetCursorPosX((30.0f - 12.0f) * scale);
                    ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), ICON_FA_CHECK);
                    ImGui::SameLine();

                    ImVec2 txtPos = ImGui::GetCursorScreenPos();
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), q.title.c_str());

                    ImVec2 txtSize = ImGui::CalcTextSize(q.title.c_str());
                    draw->AddLine(ImVec2(txtPos.x, txtPos.y + txtSize.y / 2), ImVec2(txtPos.x + txtSize.x, txtPos.y + txtSize.y / 2), IM_COL32(100, 100, 100, 100));
                }
            }
            ImGui::PopFont();
            ImGui::PopStyleVar();

            draw->PopClipRect();
        }

        ImGui::End();
    }
    ImGui::PopStyleVar(2);
}