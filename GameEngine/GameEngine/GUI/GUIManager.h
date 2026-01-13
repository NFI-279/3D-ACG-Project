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

    // --- ANIMATION STATE VARIABLES ---
    int lastHealth = -1;           // To detect health changes
    float healthFlashTimer = 0.0f; // 0.0 = Normal, 1.0 = Max Color
    float healthShakeTimer = 0.0f; // 0.0 = Still, >0 = Shaking
    int healthChangeDir = 0;       // -1 = Damage (Red), 1 = Heal (Green)

    float displayedScore = 0.0f;   // The floating point number used for the "rolling" effect
    float displayedHealth = 100.0f; // <--- NEW: Allows us to animate the number

    // Mechanic Variables
    int antidoteCount = 0;       // Starts at 0
    int backpackCount = 0;       // Current trash collected
    int maxBackpack = 5;         // Capacity

    float antidoteScaleTimer = 0.0f; // Used for Pop (Craft) and Shrink (Shoot)
    float antidoteScaleDir = 0.0f;   // 1.0 = Pop (Grow), -1.0 = Recoil (Shrink)
    float backpackPulseTimer = 0.0f; // Flash Gold when collecting trash


    // DATA kept for testing for now
    int playerHealth = 100;
    int maxHealth = 100;
    int playerScore = 30;
    int currentAmmo = 5;
    int maxAmmo = 120;


    bool showGUI = false;
    bool changeBackground = false;

    // Scene controls that we referenced in main
    bool dayNightCycle = false;
    bool enableAnimation = false;

    // Log storage
    std::vector<std::string> logs;

    void Init(GLFWwindow* window);

    void Render(const glm::vec3& playerPos, int screenWidth, int screenHeight, float fps, int renderedObjects);

    void RenderStatsHUD(float scale);

    void Shutdown();

    // Helper to add logs from anywhere
    void AddLog(const std::string& message) {
        if (logs.size() > 50) logs.erase(logs.begin()); // Keep last 50 logs
        logs.push_back(message);
    }

private:
    void drawPanel(ImVec2 startPos, ImVec2 endPos, ImU32 background_color, ImU32 outline_color, float thickness, float border_radius);
};