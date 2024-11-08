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
        auto keyCallback = [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
            engine->keyCallback(window, key);
        };
        auto mouseCallback = [](GLFWwindow* window, double xpos, double ypos)
        {
            auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
            engine->mouseCallback(window, static_cast<float>(xpos), static_cast<float>(ypos));
        };
        auto mouseButtonCallback = [](GLFWwindow* window, int button, int action, int mods)
        {
            auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
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


    void Engine::keyCallback(GLFWwindow* window, int key)
    {
        keysArePressed[key] = (glfwGetKey(window, key) == GLFW_PRESS);
    }


    void Engine::mouseCallback(GLFWwindow* window, float xpos, float ypos)
    {
        // is right-clicked basically
        if (!focused)
            return;

        // the mouse was not focused the frame before
        if (firstMouse)
        {
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


    void Engine::mouseButtonCallback(GLFWwindow* window, int button, int action)
    {
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            focused = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        {
            focused = false;
            firstMouse = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }


    void Engine::keyInput()
    {
        float deltaTime = io->DeltaTime;

        if (keysArePressed['W'] && focused)
        {
            camera.moveForward(deltaTime);
        }
        if (keysArePressed['S'] && focused)
        {
            camera.moveBackward(deltaTime);
        }
        if (keysArePressed['A'] && focused)
        {
            camera.moveLeft(deltaTime);
        }
        if (keysArePressed['D'] && focused)
        {
            camera.moveRight(deltaTime);
        }
        if (keysArePressed['Q'] && focused)
        {
            camera.moveDown(deltaTime);
        }
        if (keysArePressed[' '] && focused)
        {
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
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
            bool genChange = false;
            genChange |= ImGui::SliderFloat("Height noise scale", &config->grassUniform.sizeNoiseFrequency, 0.02, 0.7,
                                            "%.2f");
            genChange |= ImGui::SliderFloat("Base height", &config->grassUniform.bladeHeight, 0.1, 2.0, "%.1f");
            genChange |= ImGui::SliderFloat("Height Delta", &config->grassUniform.sizeNoiseAmplitude, 0.05, 0.60,
                                            "%.2f");
            if (genChange)
            {
                computeManager->generate();
            }
            ImGui::End();

            ImGui::Begin("Wind settings");
            bool movChange = false;
            movChange |= ImGui::SliderFloat3("Direction", &config->movUniform.wind.r, -1.0, 1.0, "%.1f");
            movChange |= ImGui::SliderFloat("Strength", &config->movUniform.wind.w, 0.01, 1.0, "%.2f");
            movChange |= ImGui::SliderFloat("Frequency", &config->movUniform.windFrequency, 0.01, 2.0, "%.2f");
            if (movChange)
            {
                computeManager->updateMovSettingsUniorm();
            }
            ImGui::End();


            ImGui::Begin("Blade material settings");
            bool bladeChange = false;
            bladeChange |= ImGui::ColorEdit3("Smaller blades Color", &config->bladeUniform.smallerBladeCol.r, 0);
            bladeChange |= ImGui::ColorEdit3("Taller blades Color", &config->bladeUniform.tallerBladeCol.r, 0);
            bladeChange |= ImGui::ColorEdit3("Specular Color", &config->bladeUniform.specularCol.r, 0);
            bladeChange |= ImGui::SliderFloat("Ambient", &config->bladeUniform.ambientStrength, 0.0, 1.0, "%.2f");
            bladeChange |= ImGui::SliderFloat("Diffuse", &config->bladeUniform.diffuseStrength, 0.0, 1.0, "%.2f");
            bladeChange |= ImGui::SliderFloat("Wrap value", &config->bladeUniform.wrapValue, 0.0, 1.0, "%.2f");
            bladeChange |= ImGui::SliderFloat("Specular", &config->bladeUniform.specularStrength, 0.0, 0.3, "%.3f");
            if (bladeChange)
            {
                renderer->updateBladeUniforms(config->bladeUniform);
            }
            ImGui::End();

            ImGui::Begin("Light settings");
            ImGui::SliderFloat3("Sun direction", &config->lightUniform.sunDir.r, -1.0, 1.0, "%.1f");
            ImGui::ColorEdit3("Sun color", &config->lightUniform.sunCol.r, 0);
            ImGui::ColorEdit3("Sky up color", &config->lightUniform.skyUpCol.r, 0);
            ImGui::ColorEdit3("Sky ground color", &config->lightUniform.skyGroundCol.r, 0);

            ImGui::SliderFloat("Ray max distance", &config->sssUniform.ray_max_distance, 0.0, 2.0, "%.3f");
            ImGui::SliderFloat("Thickness", &config->sssUniform.thickness, 0.0, 0.1, "%.5f");
            ImGui::SliderFloat("Nax delta from original depth", &config->sssUniform.max_delta_from_original_depth, 0.0, 0.01, "%.5f");
            ImGui::SliderInt("Steps", reinterpret_cast<int*>(&config->sssUniform.max_steps), 0, 32);
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
        computeManager->computeMovement(time);
        updateGUI();


        PhongMaterial testMat{loadTexture("../assets/portal_color.png")};
        auto testCubeGeo = MeshGeomoetry("../assets/portal.obj");
        auto testCubeMesh = Mesh(testCubeGeo, testMat);
        testCubeMesh.model = glm::translate(testCubeMesh.model, glm::vec3{0.61, 0.21, 0.5});
        testCubeMesh.model = glm::scale(testCubeMesh.model, glm::vec3{0.3});
        std::vector<Mesh> scene = {testCubeMesh};


        while (!glfwWindowShouldClose(window))
        {
            time += io->DeltaTime;

            keyInput();
            glfwPollEvents();
            camera.updateMatrix();

            computeManager->computeMovement(time);
            renderer->render(scene, camera, time);

            updateGUI();
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
