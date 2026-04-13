# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

**Build tool:** Visual Studio 2025 (MSVC v145), solution file `ParusEngine.sln`.

- Open `ParusEngine.sln` in Visual Studio and build from there (Debug/Release, Win32/x64).
- Output lands in `build\$(Platform)\` (e.g., `build\x64\`).
- Intermediate files go to `intermediate\$(Platform)\`.

**Shader compilation** — run `compile_shaders.bat` to compile GLSL shaders to SPIR-V. It uses `glslc.exe` from the Vulkan SDK and writes `.spv` files to `bin\shaders\`.

**Prerequisites:**
- Visual Studio 2022 with C++ workload
- Vulkan SDK installed with `VULKAN_SDK` environment variable set

There is no CMake and no automated test suite.

## Architecture

### Application Lifecycle

`Main.cpp` creates an `Application` instance and calls three methods:
1. `app.init()` — registers all services, loads config, creates the window, initialises Vulkan and ImGui.
2. `app.loop()` — runs the main loop: delta time, world tick, platform messages, ImGui + Vulkan frame.
3. `app.clean()` — ordered shutdown.

### Service Locator

`source/services/Services.h` is the dependency-injection layer. All major subsystems are registered as `Service` subclasses and retrieved by type:

```cpp
Services::registerService(configs.get(), configs);
auto config = Services::get<Configs>();
```

The eight core services are: `Platform`, `Configs`, `VulkanRenderer`, `ImGuiLibrary`, `Input`, `World`, `ThreadPool`, and the event system.

### Event System

`source/engine/Event.h` — type-safe, observer-style events backed by `std::any`. Use `REGISTER_EVENT()` and `FIRE_EVENT()` macros.

### Vulkan Rendering

The renderer lives in `source/services/renderer/vulkan/`. Key concepts:

- **Builder pattern** — every Vulkan object (`VkInstance`, `VkDevice`, swap chain, pipeline, image, descriptor pool/layout, render pass, texture…) has its own `*Builder` class in `builder/`. Add new Vulkan resources by creating a matching builder.
- **VulkanStorage** — acts as a resource cache; holds handles to all live Vulkan objects (pipelines, descriptor sets, buffers, images, etc.).
- **Two-pass rendering** — skybox pass then main scene pass.
- **Global geometry buffers** — vertices and indices are pooled; mesh instances reference offsets into these buffers.
- **Max 2 frames in flight** with per-frame sync primitives.

### World / Scene

`source/services/world/` — `World` owns a `Storage` container (entities, mesh instances, lights) and a `SpectatorCamera`. The renderer pulls from `World` each frame.

### Configuration

INI files under `config/`. Loaded via `Configs` service:

```cpp
configs->get("Window", "width");
configs->getAsInt("Window", "width").value_or(1200);
```

### Threading

`source/services/threading/ThreadPool.h` — 3 worker threads. Fire-and-forget with `RUN_ASYNC(fn)`.

### Third-Party Libraries (bundled in `source/third-party/`)

- **ImGui** — UI, with `imgui_impl_vulkan` and `imgui_impl_win32` backends.
- **stb_image** — texture loading.
- **tiny_obj_loader** — OBJ mesh loading.

## Conventions

- C++20, MSVC extensions enabled.
- Platform-detection macros are in `source/engine/Defines.h`; engine-wide includes in `source/engine/EngineCore.h`.
- Windows-only for now; platform abstraction (`source/services/platform/Platform.h`) exists to support future porting.
- Shader source lives in `source/shaders/`; compiled SPIR-V lives in `bin/shaders/`.