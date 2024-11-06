#include "Engine.h"
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "Utils.h"

#include <cassert>
#include <iostream>
#include <glm/glm.hpp>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include "Mesh.h"

namespace grass
{
    Engine* loadedEngine = nullptr;
    Engine& Engine::Get() { return *loadedEngine; }


    void Engine::init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        keysArePressed = new bool[512]{false};
        config = std::make_shared<GlobalConfig>();

        initWindow();
        GPUContext::getInstance(window)->configSurface(WIDTH, HEIGHT);

        computeManager = std::make_unique<ComputeManager>(config);
        wgpu::Buffer computeBuffer = computeManager->init();

        renderer = std::make_unique<Renderer>(config, WIDTH, HEIGHT);
        renderer->init(computeBuffer);

        initGUI();
    }


    void Engine::initWindow()
    {
        assert(glfwInit() && "Could not initialize GLFW!");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Grass renderer", nullptr, nullptr);
        assert(window && "Could not get window from GLFW!");

        glfwSetWindowUserPointer(window, this);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        auto keyCallback = [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
            engine->keyCallback(window, key);
        };
        auto mouseCallback = [](GLFWwindow *window, double xpos, double ypos) {
            auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
            engine->mouseCallback(window, static_cast<float>(xpos), static_cast<float>(ypos));
        };
        auto mouseButtonCallback = [](GLFWwindow *window, int button, int action, int mods) {
            auto *engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
            engine->mouseButtonCallback(window, button, action);
        };
        glfwSetCursorPosCallback(window, mouseCallback);
        glfwSetKeyCallback(window, keyCallback);
        glfwSetMouseButtonCallback(window, mouseButtonCallback);
    }


    void Engine::initGUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = &ImGui::GetIO();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOther(window, true);
        ImGui_ImplWGPU_InitInfo info = {};
        info.Device = GPUContext::getInstance()->getDevice().Get();
        info.NumFramesInFlight = 3;
        info.RenderTargetFormat = static_cast<WGPUTextureFormat>(GPUContext::getInstance()->getSurfaceFormat());
        ImGui_ImplWGPU_Init(&info);
    }


    void Engine::keyCallback(GLFWwindow *window, int key) {
        keysArePressed[key] = (glfwGetKey(window, key) == GLFW_PRESS);
    }


    void Engine::mouseCallback(GLFWwindow *window, float xpos, float ypos) {

        // is right-clicked basically
        if (!focused)
            return;

        // the mouse was not focused the frame before
        if (firstMouse) {
            lastMousePosition.x = xpos;
            lastMousePosition.y = ypos;
            firstMouse = false;
        }
        float xOffset = xpos - lastMousePosition.x;
        float yOffset = lastMousePosition.y - ypos;

        lastMousePosition.x = xpos;
        lastMousePosition.y = ypos;

        camera.updateCamDirection(xOffset, yOffset);
    }


    void Engine::mouseButtonCallback(GLFWwindow *window, int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
            focused = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
            focused = false;
            firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

    }


    void Engine::keyInput() {
        float deltaTime = io->DeltaTime;

        if (keysArePressed['W'] && focused) {
            camera.moveForward(deltaTime);
        }
        if (keysArePressed['S'] && focused) {
            camera.moveBackward(deltaTime);
        }
        if (keysArePressed['A'] && focused) {
            camera.moveLeft(deltaTime);
        }
        if (keysArePressed['D'] && focused) {
            camera.moveRight(deltaTime);
        }
        if (keysArePressed['Q'] && focused) {
            camera.moveDown(deltaTime);
        }
        if (keysArePressed[' '] && focused) {
            camera.moveUp(deltaTime);
        }
    }


    void Engine::updateGUI()
    {
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Generation settings");
            ImGui::Text("Number of blades : %i", config->totalBlades);
            bool genChange = false;
            if (ImGui::CollapsingHeader("Field", ImGuiTreeNodeFlags_DefaultOpen)) {
                genChange |= ImGui::SliderFloat("Size noise scale", &config->grassUniform.sizeNoiseFrequency, 0.02, 0.7, "%.2f");
            }
            if (ImGui::CollapsingHeader("Height", ImGuiTreeNodeFlags_DefaultOpen)) {
                genChange |= ImGui::SliderFloat("Base", &config->grassUniform.bladeHeight, 0.1, 2.0, "%.1f");
                genChange |= ImGui::SliderFloat("Delta", &config->grassUniform.sizeNoiseAmplitude, 0.05, 0.60, "%.2f");
            }
            if (genChange) {
                computeManager->generate();
            }
            ImGui::End();

            ImGui::Begin("Wind and movement settings");
            bool movChange = false;
            if (ImGui::CollapsingHeader("Wind", ImGuiTreeNodeFlags_DefaultOpen)) {
                movChange |= ImGui::SliderFloat3("Direction (don't touch y)", &config->movUniform.wind.r, -1.0, 1.0, "%.1f");
                movChange |= ImGui::SliderFloat("Strength", &config->movUniform.wind.w, 0.01, 1.0, "%.2f");
                movChange |= ImGui::SliderFloat("Frequency", &config->movUniform.windFrequency, 0.01, 2.0, "%.2f");
            }
            if (movChange) {
                computeManager->updateMovSettingsUniorm();
            }
            ImGui::End();


            ImGui::Begin("Blades settings");
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);

            bool bladeChange = false;
            if (ImGui::CollapsingHeader("Light", ImGuiTreeNodeFlags_DefaultOpen)) {
                bladeChange |= ImGui::SliderFloat3("Direction", &config->bladeUniform.lightDirection.r, -1.0, 1.0, "%.1f");
                bladeChange |= ImGui::ColorEdit3("Color", &config->bladeUniform.lightCol.r, 0);
            }
            if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
                bladeChange |= ImGui::ColorEdit3("Blade Color", &config->bladeUniform.bladeCol.r, 0);
                bladeChange |= ImGui::ColorEdit3("Specular Color", &config->bladeUniform.specularCol.r, 0);
                bladeChange |= ImGui::SliderFloat("Ambient", &config->bladeUniform.ambientStrength, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Diffuse", &config->bladeUniform.diffuseStrength, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Wrap value", &config->bladeUniform.wrapValue, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Specular", &config->bladeUniform.specularStrength, 0.0, 0.3, "%.3f");
            }
            if (bladeChange) {
                renderer->updateBladeUniforms(config->bladeUniform);
            }
            ImGui::End();
        }
        ImGui::EndFrame();
        ImGui::Render();
    }



    void Engine::run()
    {
        computeManager->generate();
        computeManager->updateMovSettingsUniorm();
        renderer->updateBladeUniforms(config->bladeUniform);


        PhongMaterial testMat{loadTexture("../assets/test_cube_diff.png")};
        auto testCubeGeo = MeshGeomoetry("../assets/test_cube.obj");
        auto testCubeMesh = Mesh(testCubeGeo, testMat);
        std::vector<Mesh> scene = {testCubeMesh};

        while (!glfwWindowShouldClose(window))
        {
            time += io->DeltaTime;

            keyInput();
            glfwPollEvents();
            camera.updateMatrix();
            renderer->updateGlobalUniforms(camera, time);

            updateGUI();
            renderer->draw(scene);
            computeManager->computeMovement(time);

            frameNumber++;
        }
    }


    void Engine::cleanup()
    {
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplWGPU_Shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
} // grass
