#pragma once
#include <vector>
#include <string>
#include "imgui.h"
#include <glew.h>
#include <glfw3.h>
#include <glm.hpp>
#include "QuestManager.h"

class GUIManager
{
public:

    ImFont* fontHeader = nullptr; // Font for "Game Control Panel"
	ImFont* fontUI = nullptr;     // Default UI font - buttons, text
    ImFont* fontSmall = nullptr; // For Section Headers
    ImFont* fontMono = nullptr; // For FPS, Logs, Coords

    QuestManager questManager;

    bool showGUI = false;
    bool changeBackground = false;

    // Scene controls that we referenced in main
    bool dayNightCycle = false;
    bool enableAnimation = false;

    // Log storage
    std::vector<std::string> logs;

    void Init(GLFWwindow* window);

    void Render(const glm::vec3& playerPos, int screenWidth, int screenHeight, float fps, int renderedObjects);

    void Shutdown();

    // Helper to add logs from anywhere
    void AddLog(const std::string& message) {
        if (logs.size() > 50) logs.erase(logs.begin()); // Keep last 50 logs
        logs.push_back(message);
    }

private:
    void drawPanel(ImVec2 startPos, ImVec2 endPos, ImU32 background_color, ImU32 outline_color, float thickness, float border_radius);
};