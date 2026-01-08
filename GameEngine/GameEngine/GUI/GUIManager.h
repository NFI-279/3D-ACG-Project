#pragma once
#include "imgui.h"
#include <glfw3.h>

class GUIManager
{
public:
    GUIManager() = default;
    ~GUIManager() = default;

    // Initialize ImGui and pass GLFW window
    void Init(GLFWwindow* window);

    // Render the GUI each frame
    void Render();

    // Shutdown ImGui
    void Shutdown();

    // Variables exposed to main.cpp to affect scene
    bool changeBackground = false;
    bool showGUI = false;

    // Debug Info Here
    int visibleObjects = 0;
};
