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

# Headers are not compiled, but listing them here lets IDEs
# (Visual Studio, CLion, Rider) show them in the project tree.
target_sources(ParusEngineLib PRIVATE
    source/engine/Asserts.h
    source/engine/Defines.h
    source/engine/EngineCore.h
    source/engine/Event.h
    source/engine/application/Application.h
    source/engine/input/Input.h
    source/engine/logs/Logs.h
    source/engine/utils/Utils.h
    source/engine/utils/math/Math.h
    source/engine/utils/math/UniformBufferObjects.h
    source/services/Service.h
    source/services/Services.h
    source/services/config/ConfigError.h
    source/services/config/ConfigParser.h
    source/services/config/Configs.h
    source/services/console/Console.h
    source/services/console/Trie.h
    source/services/graphics/GraphicsLibrary.h
    source/services/graphics/gui/ConsoleGui.h
    source/services/graphics/imgui/ImGuiLibrary.h
    source/services/graphics/imgui/Theme.h
    source/services/platform/Platform.h
    source/services/platform/PlatformWindows.h
    source/services/renderer/Material.h
    source/services/renderer/Renderer.h
    source/services/renderer/Texture.h
    source/services/renderer/TextureType.h
    source/services/renderer/vulkan/GraphicsOverlay.h
    source/services/renderer/vulkan/VulkanConfigurator.h
    source/services/renderer/vulkan/VulkanDescriptor.h
    source/services/renderer/vulkan/VulkanDescriptorManager.h
    source/services/renderer/vulkan/VulkanInitializer.h
    source/services/renderer/vulkan/VulkanRenderer.h
    source/services/renderer/vulkan/builder/VkBufferBuilder.h
    source/services/renderer/vulkan/builder/VkCommandBufferBuilder.h
    source/services/renderer/vulkan/builder/VkCommandPoolFactory.h
    source/services/renderer/vulkan/builder/VkDebugUtilsBuilder.h
    source/services/renderer/vulkan/builder/VkDescriptorPoolBuilder.h
    source/services/renderer/vulkan/builder/VkDescriptorSetLayoutBuilder.h
    source/services/renderer/vulkan/builder/VkDeviceFactory.h
    source/services/renderer/vulkan/builder/VkDeviceMemoryBuilder.h
    source/services/renderer/vulkan/builder/VkFramebufferBuilder.h
    source/services/renderer/vulkan/builder/VkImageBuilder.h
    source/services/renderer/vulkan/builder/VkImageViewBuilder.h
    source/services/renderer/vulkan/builder/VkInstanceBuilder.h
    source/services/renderer/vulkan/builder/VkPipelineBuilder.h
    source/services/renderer/vulkan/builder/VkQueuesFactory.h
    source/services/renderer/vulkan/builder/VkRenderPassFactory.h
    source/services/renderer/vulkan/builder/VkSamplerBuilder.h
    source/services/renderer/vulkan/builder/VkSurfaceFactory.h
    source/services/renderer/vulkan/builder/VkSwapChainFactory.h
    source/services/renderer/vulkan/builder/VkSyncObjectsFactory.h
    source/services/renderer/vulkan/builder/VkUboBuilder.h
    source/services/renderer/vulkan/builder/VulkanTexture2dBuilder.h
    source/services/renderer/vulkan/light/Light.h
    source/services/renderer/vulkan/material/VulkanMaterial.h
    source/services/renderer/vulkan/mesh/Mesh.h
    source/services/renderer/vulkan/mesh/MeshInstance.h
    source/services/renderer/vulkan/mesh/SkyboxMesh.h
    source/services/renderer/vulkan/pass/DepthPrePass.h
    source/services/renderer/vulkan/pass/MainPass.h
    source/services/renderer/vulkan/pass/SSAOBlurPass.h
    source/services/renderer/vulkan/pass/SSAOPass.h
    source/services/renderer/vulkan/pass/ShadowPass.h
    source/services/renderer/vulkan/pass/VulkanRenderPass.h
    source/services/renderer/vulkan/storage/VulkanStorage.h
    source/services/renderer/vulkan/texture/VulkanTexture2d.h
    source/services/renderer/vulkan/utils/VulkanUtils.h
    source/services/serialization/BinaryStream.h
    source/services/serialization/FormatHeader.h
    source/services/serialization/MeshFormat.h
    source/services/serialization/SceneData.h
    source/services/serialization/Serialization.h
    source/services/serialization/TextureFormat.h
    source/services/serialization/WorldFormat.h
    source/services/threading/ThreadPool.h
    source/services/world/SceneLight.h
    source/services/world/Storage.h
    source/services/world/World.h
    source/services/world/WorldMeshInstance.h
    source/services/world/camera/SpectatorCamera.h
    source/third-party/imgui/backends/imgui_impl_vulkan.h
    source/third-party/imgui/backends/imgui_impl_win32.h
    source/third-party/imgui/imconfig.h
    source/third-party/imgui/imgui.h
    source/third-party/imgui/imgui_internal.h
    source/third-party/imgui/imstb_rectpack.h
    source/third-party/imgui/imstb_textedit.h
    source/third-party/imgui/imstb_truetype.h
    source/third-party/stb_image.h
    source/third-party/stb_image_write.h
    source/third-party/tiny_obj_loader.h
)

target_include_directories(ParusEngineLib PUBLIC
        source
        source/third-party/imgui
)

target_link_libraries(ParusEngineLib PUBLIC Vulkan::Vulkan)

# C++20, and don't fall back to an older standard if unavailable.
target_compile_features(ParusEngineLib PUBLIC cxx_std_23)

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
