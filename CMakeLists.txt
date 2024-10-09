set(USE_VULKAN ON)
set(USE_D3D12 OFF)

set(DAWN_FETCH_DEPENDENCIES ON)
set(DAWN_BUILD_SAMPLES ON)
set(TINT_BUILD_CMD_TOOLS OFF)
set(TINT_BUILD_TESTS OFF)
set(TINT_BUILD_IR_BINARY OFF)

set(DAWN_ENABLE_D3D11 OFF)
set(DAWN_ENABLE_D3D12 ${USE_D3D12})
set(DAWN_ENABLE_NULL OFF)
set(DAWN_ENABLE_DESKTOP_GL OFF)
set(DAWN_ENABLE_OPENGLES OFF)
set(DAWN_ENABLE_VULKAN ${USE_VULKAN})
set(TINT_BUILD_SPV_READER OFF)
set(DAWN_ENABLE_EMSCRIPTEN OFF)


cmake_minimum_required(VERSION 3.28)
project(GrassRenderer VERSION 0.1.0)

set(PROJECT_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LIBS_DIR "${PROJECT_ROOT_DIR}/third_party")
set(SOURCE_DIR "${PROJECT_ROOT_DIR}/src")

file(GLOB_RECURSE SOURCES
        "${SOURCE_DIR}/*.cpp"
        "${SOURCE_DIR}/*.c"
)

add_executable(App main.cpp ${SOURCES})

set_target_properties(App PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        COMPILE_WARNING_AS_ERROR ON
)

add_subdirectory(dawn EXCLUDE_FROM_ALL)

function(target_copy_webgpu_binaries Target)
    add_custom_command(
            TARGET ${Target} POST_BUILD
            COMMAND
            "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE_DIR:webgpu_dawn>/$<TARGET_FILE_NAME:webgpu_dawn>"
            "$<TARGET_FILE_DIR:${Target}>"
            COMMENT
            "Copying '$<TARGET_FILE_DIR:webgpu_dawn>/$<TARGET_FILE_NAME:webgpu_dawn>' to '$<TARGET_FILE_DIR:${Target}>'..."
    )
endfunction()

target_link_libraries(App PRIVATE dawn::webgpu_dawn glfw webgpu_glfw)
target_copy_webgpu_binaries(App)



