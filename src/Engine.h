#pragma once

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#include <dawn/native/DawnNative.h>
#include <imgui.h>

#include "Camera.h"
#include "Renderer.h"
#include "ComputeManager.h"
#include "GlobalConfig.h"
#include "GPUContext.h"


namespace grass
{
    class Engine
    {

    const uint16_t WIDTH = 1600;
    const uint16_t HEIGHT = 900;

    public:
        static Engine& getInstance();

        void init();
        void run();
        void cleanup();

    private:
        void initWindow();
        void initGUI();
        void updateGUI();
        void keyCallback(GLFWwindow* window, int key);
        void mouseCallback(GLFWwindow* window, float xpos, float ypos);
        void mouseButtonCallback(GLFWwindow *window,  int button, int action);
        void keyInput();

        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<ComputeManager> computeManager;

        std::shared_ptr<GlobalConfig> config;

        GLFWwindow* window = nullptr;
        ImGuiIO *io = nullptr;
        Camera camera{35.0, static_cast<float>(WIDTH) / static_cast<float>(HEIGHT)};
        uint32_t frameNumber = 0;
        float time = 0.0;

        // Controls
        bool focused = false;
        bool *keysArePressed = nullptr;
        bool isFirstMouseMove = true;
        glm::vec2 lastMousePosition = glm::vec2();
    };
} // grass
