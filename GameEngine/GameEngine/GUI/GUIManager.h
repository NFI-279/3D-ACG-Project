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

    bool isGamePaused = false;     // Toggled by ESC
    bool isFullscreen = true;      // Tracks window state

    float crosshairAlpha = 1.0f; // 1.0 = Visible, 0.0 = Hidden

    void Init(GLFWwindow* window);
    void Shutdown();

    void TriggerShootAnimation(); // For Syringe
    void TriggerGunShootAnimation(); // For Gun

    void Render(const glm::vec3& playerPos, int screenWidth, int screenHeight, float fps, int renderedObjects,
        int currentScore, int currentHealth, int currentBackpack, int currentAntidotes, int currentGunAmmo);

    void RenderPauseMenu(float scale);

    void AddLog(const std::string& message) {
        if (logs.size() > 50) logs.erase(logs.begin()); // Keep last 50 logs
        logs.push_back(message);
    }

    void RenderVictoryScreen(float scale);
    void RenderDeathScreen(float scale);

    // crosshair at the center of the screen
    void DrawCrosshair(float scale);

private:
    // Synced from Main
    int playerHealth = 100;
    int playerScore = 30;
    int antidoteCount = 0;
    int backpackCount = 0;
    int gunAmmoCount = 0;

    // Constants
    const int maxHealth = 100;
    const int maxBackpack = 5;
    const int maxGunAmmo = 15;

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
    float gunScaleTimer = 0.0f;
    float gunScaleDir = 0.0f;

    // Fonts
    ImFont* fontHeader = nullptr;
    ImFont* fontUI = nullptr;
    ImFont* fontSmall = nullptr;
    ImFont* fontMono = nullptr;

    // Log Storage
    std::vector<std::string> logs;

    void RenderStatsHUD(float scale);
};