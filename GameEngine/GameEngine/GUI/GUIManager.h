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
    QuestManager questManager;

    // Read/Write by Main
    bool showGUI = false;          // Toggled by Insert
    bool changeBackground = false; // Debug visual
    bool dayNightCycle = false;    // Scene Control
    bool enableAnimation = false;  // Scene Control

    void Init(GLFWwindow* window);
    void Shutdown();

    void Render(const glm::vec3& playerPos, int screenWidth, int screenHeight, float fps, int renderedObjects,
        int currentScore, int currentHealth, int currentBackpack, int currentAntidotes);

    void AddLog(const std::string& message) {
        if (logs.size() > 50) logs.erase(logs.begin()); // Keep last 50 logs
        logs.push_back(message);
    }

private:
    // Synced from Main
    int playerHealth = 100;
    int playerScore = 30;
    int antidoteCount = 0;
    int backpackCount = 0;

    // Constants
    const int maxHealth = 100;
    const int maxBackpack = 5;

    // Animation State
    // Health
    int lastHealth = -1;
    float healthFlashTimer = 0.0f;
    float healthShakeTimer = 0.0f;
    int healthChangeDir = 0;
    float displayedHealth = 100.0f;

    // Score
    float displayedScore = 0.0f;

    // Items
    float antidoteScaleTimer = 0.0f;
    float antidoteScaleDir = 0.0f;
    float backpackPulseTimer = 0.0f;

    // Fonts
    ImFont* fontHeader = nullptr;
    ImFont* fontUI = nullptr;
    ImFont* fontSmall = nullptr;
    ImFont* fontMono = nullptr;

    // Log Storage
    std::vector<std::string> logs;

    void RenderStatsHUD(float scale);
};