#include "Engine.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include <glm/glm.hpp>
#include <backends/imgui_impl_wgpu.h>
#include <backends/imgui_impl_glfw.h>

#include <cassert>

#include "Mesh.h"
#include "Utils.h"


namespace grass
{
    Engine* loadedEngine = nullptr;
    Engine& Engine::getInstance() { return *loadedEngine; }


    bool Engine::init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        keysArePressed = new bool[512]{false};
        config = std::make_shared<GlobalConfig>();

        if (!initWindow()) return false;
        GPUContext::getInstance(window)->configSurface(WIDTH, HEIGHT);

        computeManager = std::make_unique<ComputeManager>(config);
        renderer = std::make_unique<Renderer>(config, WIDTH, HEIGHT);

        wgpu::Buffer computeBuffer = computeManager->init();
        if (computeBuffer == nullptr) return false;
        if (!renderer->init(computeBuffer)) return false;
        if (!initGUI()) return false;

        return true;
    }


    bool Engine::initWindow()
    {
        if (!glfwInit())
        {
            std::cerr << "Could not initialize GLFW!" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Grass renderer", nullptr, nullptr);
        if (window == nullptr)
        {
            std::cerr << "Could not get window from GLFW!" << std::endl;
            return false;
        }

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

        return true;
    }


    bool Engine::initGUI()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        io = &ImGui::GetIO();
        if (io == nullptr)
        {
            std::cerr << "Could not get imgui IO!" << std::endl;
            return false;
        }
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOther(window, true);
        ImGui_ImplWGPU_InitInfo info = {};
        info.Device = GPUContext::getInstance()->getDevice().Get();
        info.NumFramesInFlight = 3;
        info.RenderTargetFormat = static_cast<WGPUTextureFormat>(GPUContext::getInstance()->getSurfaceFormat());
        ImGui_ImplWGPU_Init(&info);

        return true;
    }


    void Engine::keyCallback(GLFWwindow* window, int key)
    {
        keysArePressed[key] = (glfwGetKey(window, key) == GLFW_PRESS);
    }


    void Engine::mouseCallback(GLFWwindow* window, float xpos, float ypos)
    {
        if (!focused)
            return;

        // the mouse was not focused the frame before
        if (isFirstMouseMove)
        {
            lastMousePosition.x = xpos;
            lastMousePosition.y = ypos;
            isFirstMouseMove = false;
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
            isFirstMouseMove = true;
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
        if (keysArePressed['G'] && focused)
        {
            renderer->toggleGUI();
        }
    }


    void Engine::updateGUI()
    {
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        {
            ImGui::Begin("Settings");
            ImGui::Text("Number of blades : %i", config->totalBlades);
            ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / io->Framerate, io->Framerate);
            if (ImGui::CollapsingHeader("Generation", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool genChange = false;
                genChange |= ImGui::SliderFloat("Height noise scale", &config->grassUniform.sizeNoiseFrequency, 0.02,
                                                0.7,
                                                "%.2f");
                genChange |= ImGui::SliderFloat("Base height", &config->grassUniform.bladeHeight, 0.1, 2.0, "%.1f");
                genChange |= ImGui::SliderFloat("Height Delta", &config->grassUniform.sizeNoiseAmplitude, 0.05, 0.60,
                                                "%.2f");
                if (genChange)
                {
                    computeManager->generate();
                }
            }

            if (ImGui::CollapsingHeader("Wind", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool movChange = false;
                movChange |= ImGui::SliderFloat3("Direction", &config->movUniform.wind.r, -1.0, 1.0, "%.1f");
                movChange |= ImGui::SliderFloat("Strength", &config->movUniform.wind.w, 0.01, 1.0, "%.2f");
                movChange |= ImGui::SliderFloat("Frequency", &config->movUniform.windFrequency, 0.01, 2.0, "%.2f");
                if (movChange)
                {
                    computeManager->updateMovSettingsUniorm();
                }
            }

            if (ImGui::CollapsingHeader("Blade material", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool bladeChange = false;
                bladeChange |= ImGui::ColorEdit3("Smaller blades Color", &config->bladeUniform.smallerBladeCol.r, 0);
                bladeChange |= ImGui::ColorEdit3("Taller blades Color", &config->bladeUniform.tallerBladeCol.r, 0);
                bladeChange |= ImGui::ColorEdit3("Specular Color", &config->bladeUniform.specularCol.r, 0);
                bladeChange |= ImGui::SliderFloat("Ambient", &config->bladeUniform.ambientStrength, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Diffuse", &config->bladeUniform.diffuseStrength, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Wrap value", &config->bladeUniform.wrapValue, 0.0, 1.0, "%.2f");
                bladeChange |= ImGui::SliderFloat("Specular", &config->bladeUniform.specularStrength, 0.0, 0.3, "%.3f");
                bladeChange |= ImGui::SliderFloat("Use shadows", &config->bladeUniform.shadows, 0.0, 1.0, "%1.f");
                if (bladeChange)
                {
                    renderer->updateBladeUniforms();
                }
            }
            if (ImGui::CollapsingHeader("Light settings", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::SliderFloat3("Sun direction", &config->lightUniform.sunDir.r, -1.0, 1.0, "%.1f");
                ImGui::ColorEdit3("Sun color", &config->lightUniform.sunCol.r, 0);
                ImGui::ColorEdit3("Sky up color", &config->lightUniform.skyUpCol.r, 0);
                ImGui::ColorEdit3("Sky ground color", &config->lightUniform.skyGroundCol.r, 0);
            }
            if (ImGui::CollapsingHeader("Screen space shadows", ImGuiTreeNodeFlags_DefaultOpen))
            {
                bool shadowChange = false;
                shadowChange |= ImGui::SliderFloat("Ray max distance", &config->shadowUniform.ray_max_distance, 0.0,
                                                   3.0, "%.3f");
                shadowChange |= ImGui::SliderFloat("Thickness", &config->shadowUniform.thickness, 0.0, 0.5, "%.5f");
                shadowChange |= ImGui::SliderFloat("Nax delta from original depth",
                                                   &config->shadowUniform.max_delta_from_original_depth, 0.0, 0.03,
                                                   "%.5f");
                shadowChange |= ImGui::SliderInt("Steps", reinterpret_cast<int*>(&config->shadowUniform.max_steps), 0,
                                                 32);
                if (shadowChange)
                {
                    renderer->updateShadowUniforms();
                }
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
        computeManager->computeMovement(time);
        updateGUI();

        // Little preview scene
        PhongMaterial portalCol{loadTexture("../assets/portal_color.png")};
        auto portalGeo = MeshGeomoetry("../assets/portal.obj");
        auto portalMesh = Mesh(portalGeo, portalCol);
        portalMesh.model = glm::translate(portalMesh.model, glm::vec3{0.61, 0.12, 0.5});
        portalMesh.model = glm::scale(portalMesh.model, glm::vec3{0.3});

        const auto scene = {portalMesh};

        while (!glfwWindowShouldClose(window))
        {
            time += io->DeltaTime;

            keyInput();
            glfwPollEvents();
            camera.updateMatrix();

            computeManager->computeMovement(time);
            renderer->render(scene, camera, time, frameNumber);
            updateGUI();

            frameNumber++;
        }
    }


    void Engine::cleanup()
    {
        delete [] keysArePressed;
        ImGui_ImplGlfw_Shutdown();
        ImGui_ImplWGPU_Shutdown();
        glfwDestroyWindow(window);
        glfwTerminate();
    }
} // grass
