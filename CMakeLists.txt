cmake_minimum_required(VERSION 3.20)

# ---- ParusEngine ----
project(ParusEngine LANGUAGES CXX)

find_package(Vulkan REQUIRED)

add_library(ParusEngineLib STATIC)

target_sources(ParusEngineLib PRIVATE
    source/engine/application/Application.cpp
    source/engine/input/Input.cpp
    source/engine/logs/Logs.cpp
    source/engine/utils/math/Math.cpp
    source/services/Services.cpp
    source/services/config/Configs.cpp
    source/services/console/Console.cpp
    source/services/graphics/GraphicsLibrary.cpp
    source/services/graphics/gui/ConsoleGui.cpp
    source/services/graphics/imgui/ImGuiLibrary.cpp
    source/services/platform/PlatformWindows.cpp
    source/services/renderer/Renderer.cpp
    source/services/renderer/vulkan/VulkanDescriptor.cpp
    source/services/renderer/vulkan/VulkanDescriptorManager.cpp
    source/services/renderer/vulkan/VulkanInitializer.cpp
    source/services/renderer/vulkan/VulkanRenderer.cpp
    source/services/renderer/vulkan/builder/VkBufferBuilder.cpp
    source/services/renderer/vulkan/builder/VkCommandBufferBuilder.cpp
    source/services/renderer/vulkan/builder/VkCommandPoolFactory.cpp
    source/services/renderer/vulkan/builder/VkDebugUtilsBuilder.cpp
    source/services/renderer/vulkan/builder/VkDescriptorPoolBuilder.cpp
    source/services/renderer/vulkan/builder/VkDescriptorSetLayoutBuilder.cpp
    source/services/renderer/vulkan/builder/VkDeviceFactory.cpp
    source/services/renderer/vulkan/builder/VkDeviceMemoryBuilder.cpp
    source/services/renderer/vulkan/builder/VkFramebufferBuilder.cpp
    source/services/renderer/vulkan/builder/VkImageBuilder.cpp
    source/services/renderer/vulkan/builder/VkImageViewBuilder.cpp
    source/services/renderer/vulkan/builder/VkInstanceBuilder.cpp
    source/services/renderer/vulkan/builder/VkPipelineBuilder.cpp
    source/services/renderer/vulkan/builder/VkQueuesFactory.cpp
    source/services/renderer/vulkan/builder/VkRenderPassFactory.cpp
    source/services/renderer/vulkan/builder/VkSamplerBuilder.cpp
    source/services/renderer/vulkan/builder/VkSurfaceFactory.cpp
    source/services/renderer/vulkan/builder/VkSwapChainFactory.cpp
    source/services/renderer/vulkan/builder/VkSyncObjectsFactory.cpp
    source/services/renderer/vulkan/builder/VkUboBuilder.cpp
    source/services/renderer/vulkan/builder/VulkanTexture2dBuilder.cpp
    source/services/renderer/vulkan/light/Light.cpp
    source/services/renderer/vulkan/material/VulkanMaterial.cpp
    source/services/renderer/vulkan/mesh/Mesh.cpp
    source/services/renderer/vulkan/mesh/MeshInstance.cpp
    source/services/renderer/vulkan/pass/DepthPrePass.cpp
    source/services/renderer/vulkan/pass/MainPass.cpp
    source/services/renderer/vulkan/pass/SSAOBlurPass.cpp
    source/services/renderer/vulkan/pass/SSAOPass.cpp
    source/services/renderer/vulkan/pass/ShadowPass.cpp
    source/services/renderer/vulkan/pass/VulkanRenderPass.cpp
    source/services/renderer/vulkan/storage/VulkanStorage.cpp
    source/services/renderer/vulkan/utils/VulkanUtils.cpp
    source/services/serialization/FormatHeader.cpp
    source/services/serialization/MeshFormat.cpp
    source/services/serialization/Serialization.cpp
    source/services/serialization/TextureFormat.cpp
    source/services/serialization/WorldFormat.cpp
    source/services/threading/ThreadPool.cpp
    source/services/world/Storage.cpp
    source/services/world/World.cpp
    source/services/world/camera/SpectatorCamera.cpp
    source/third-party/imgui/imgui.cpp
    source/third-party/imgui/imgui_draw.cpp
    source/third-party/imgui/imgui_tables.cpp
    source/third-party/imgui/imgui_widgets.cpp
    source/third-party/imgui/backends/imgui_impl_vulkan.cpp
    source/third-party/imgui/backends/imgui_impl_win32.cpp
)

target_include_directories(ParusEngineLib PUBLIC
        source
        source/third-party/imgui
)

target_link_libraries(ParusEngineLib PUBLIC Vulkan::Vulkan)

# C++20, and don't fall back to an older standard if unavailable.
target_compile_features(ParusEngineLib PUBLIC cxx_std_20)

add_executable(ParusEngine source/Main.cpp)
target_link_libraries(ParusEngine PRIVATE ParusEngineLib)

# Copy runtime data (config + bin) next to the executable so it can run
# from its own folder, independent of the repository layout.
add_custom_command(TARGET ParusEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/config"
        "$<TARGET_FILE_DIR:ParusEngine>/config"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/bin"
        "$<TARGET_FILE_DIR:ParusEngine>/bin"
        COMMENT "Staging runtime data next to ParusEngine.exe"
)

# ---- Testing ----
include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
)

# On Windows, force GoogleTest to use the same runtime library as our targets.                                    
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) 

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(ParusEngineTests
    tests/MathTests.cpp
    tests/SerializationTests.cpp
)

target_link_libraries(ParusEngineTests PRIVATE
    ParusEngineLib 
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ParusEngineTests)
