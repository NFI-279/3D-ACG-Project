#pragma once
#include <string>
#include <vector>
#include "imgui.h"

// Defines a single mission step
struct QuestTask {
    std::string title;       // Main text "Locate the Signal"
    std::string description;
    bool isCompleted = false;
    bool isActive = false;
};

class QuestManager {
public:
    void Init(); // Load the 5 story tasks

    // Pass the fonts from GUIManager so we can use the specific styles (Bold, Mono, etc.)
    void Render(ImFont* fontUI, ImFont* fontHeader, ImFont* fontMono, float scale, bool showDetails);

    // Call this to mark the current task as done and move to the next
    void CompleteCurrentQuest();

    bool IsAnnouncementActive() const { return isAnnouncementActive; }

private:
    std::vector<QuestTask> quests;
    int currentQuestIndex = -1;

    // For the expansion on hover
    float hoverAnimation = 0.0f; // 0.0 (Closed) -> 1.0 (Open)

    // For the pulsing effect
    float pulseTimer = 0.0f;

    // For the "Cinematic Center Banner"
    bool isAnnouncementActive = false;
    float announcementTimer = 0.0f;

    // Helper to add tasks internally
    void AddQuest(std::string title, std::string desc);
};