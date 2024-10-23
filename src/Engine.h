#pragma once

#define WEBGPU_CPP_IMPLEMENTATION
#include <webgpu/webgpu_cpp.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>
#include <dawn/native/DawnNative.h>
#include <imgui.h>

#include "GrassSettings.h"
#include "Camera.h"
#include "Renderer.h"
#include "ComputeManager.h"


namespace grass
{
    class Engine
    {

    public:
        static Engine& Get();

        void init();
        void run();
        void cleanup();

    private:
        void initWindow();
        void initWebgpu();
        void configSurface();
        void initGUI();
        void updateGUI(const wgpu::CommandEncoder& encoder, const wgpu::TextureView& targetView);
        wgpu::TextureView getNextSurfaceTextureView();

        void keyCallback(GLFWwindow* window, int key);
        void mouseCallback(GLFWwindow* window, float xpos, float ypos);
        void mouseButtonCallback(GLFWwindow *window,  int button, int action);
        void keyInput();

        std::unique_ptr<Renderer> renderer;
        std::unique_ptr<ComputeManager> computeManager;

        wgpu::Instance instance;
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Surface surface;
        wgpu::TextureFormat surfaceFormat = wgpu::TextureFormat::Undefined;

        GLFWwindow* window = nullptr;
        ImGuiIO *io = nullptr;
        Camera camera{35.0, WIDTH / static_cast<float_t>(HEIGHT)};
        uint32_t frameNumber = 0;
        float time = 0.0;
        GrassSettings grassSettings{};

        // Controls
        bool focused = false;
        bool *keysArePressed;
        bool firstMouse = true;
        glm::vec2 lastMousePosition = glm::vec2();
    };
} // grass
