# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

**Build tool:** CMake (>=3.20) with Ninja generator, via presets in `CMakePresets.json`. Requires an x64 MSVC dev shell (`cl.exe` on PATH) and the Vulkan SDK (`VULKAN_SDK` env var set).

```bash
cmake --preset debug          # configure (or: release)
cmake --build --preset debug  # build
ctest --preset debug          # run tests
```

- Output binaries land in `build/<preset>/bin/`; the post-build step copies `config/` and `bin/` (assets, shaders) next to `ParusEngine.exe`.
- Tests use GoogleTest (fetched via `FetchContent`, pinned to v1.15.2) and are discovered automatically with `gtest_discover_tests`. Test sources live in `tests/` (e.g. `tests/MathTests.cpp`, `tests/SerializationTests.cpp`) and are added to `CMakeLists.txt`'s `ParusEngineTests` target.
- CI (`.github/workflows/ci.yml`) runs on `windows-latest` for every push/PR to `master`: sets up MSVC x64, installs the Vulkan SDK (headers/loader only, no runtime), then configures/builds/tests the `debug` preset. Tests are GPU-free.

**Shader compilation** — run `compile_shaders.sh` (bash) to compile GLSL shaders to SPIR-V via `glslc` from the Vulkan SDK, writing `.spv` files to `bin/shaders/`. It also re-syncs `bin/` into any existing `build/*/bin/` directories.

## Architecture

### Application Lifecycle

`source/Main.cpp` creates an `Application` instance and calls three methods:
1. `app.init()` — calls `registerServices()`, loads config, creates the window, initialises Vulkan and ImGui.
2. `app.loop()` — runs the main loop: delta time, world tick, platform messages, ImGui + Vulkan frame.
3. `app.clean()` — ordered shutdown.

### Service Locator

`source/services/Services.h` is the dependency-injection layer. Services are registered by base type and retrieved by type:

```cpp
Services::registerService<Configs>(std::make_shared<Configs>());
auto config = Services::get<Configs>();
```

`Application::registerServices()` (`source/engine/application/Application.cpp`) registers ten services: `Configs`, `Platform`, `GraphicsLibrary` (backed by `imgui::ImGuiLibrary`), `Renderer` (backed by `vulkan::VulkanRenderer`), `EventSystem`, `Input`, `World`, `ThreadPool`, `Console`, and `Serialization`.

### Event System

`source/engine/Event.h` — type-safe, observer-style events backed by `std::any`. Use `REGISTER_EVENT()` and `FIRE_EVENT()` macros.

### Console

`source/services/console/Console.h` — an in-engine command console. Commands are registered with `registerConsoleCommand(name, callback)` and dispatched via `processCommand()`; `Trie` (`source/services/console/Trie.h`) backs tab-completion via `hintNext()`. `ConsoleGui` (`source/services/graphics/gui/ConsoleGui.h`) renders it through ImGui.

### Serialization

`source/services/serialization/` — a custom binary format for meshes, textures, and scenes (this is the foundation for the project's long-term "load scenes from the web" goal, see `docs/ROADMAP.md`). Key pieces:

- `FormatHeader` — shared header written at the start of each binary format.
- `MeshFormat`, `TextureFormat`, `WorldFormat` — per-asset-type (de)serializers.
- `BinaryStream` — read/write primitive for binary payloads.
- `SceneData` — in-memory representation of a loaded/saved scene.
- `Serialization` service — exposes `saveCurrentWorld(sceneName)` / `importWorld(sceneName)`, and registers `save`/`import` console commands.

### Vulkan Rendering

The renderer lives in `source/services/renderer/vulkan/`. Key concepts:

- **Builder pattern** — every Vulkan object (`VkInstance`, `VkDevice`, swap chain, pipeline, image, descriptor pool/layout, render pass, texture…) has its own `*Builder`/`*Factory` class in `builder/`. Add new Vulkan resources by creating a matching builder. Builder `build()` methods return `void`; factory-style builders expose a `getResult()` accessor instead.
- **VulkanStorage** — acts as a resource cache; holds handles to all live Vulkan objects (pipelines, descriptor sets, buffers, images, etc.).
- **Multi-pass rendering** — `pass/` contains `ShadowPass`, `DepthPrePass`, `SSAOPass`, `SSAOBlurPass`, and `MainPass`, run in that order per frame.
- **Global geometry buffers** — vertices and indices are pooled; mesh instances (`mesh/MeshInstance.h`) reference offsets into these buffers.
- **Max 2 frames in flight** with per-frame sync primitives.

### World / Scene

`source/services/world/` — `World` owns a `Storage` container (entities, mesh instances, lights) and a `SpectatorCamera`. The renderer pulls from `World` each frame. `Storage` is also what `Serialization` reads from / writes into when saving or loading a scene.

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
- **GoogleTest** — fetched by CMake at configure time (not vendored), used only by the `ParusEngineTests` target.

## Conventions

- C++20, MSVC extensions enabled.
- Platform-detection macros are in `source/engine/Defines.h`; engine-wide includes in `source/engine/EngineCore.h`.
- Windows-only for now; platform abstraction (`source/services/platform/Platform.h`) exists to support future porting.
- Shader source lives in `source/shaders/`; compiled SPIR-V lives in `bin/shaders/`.

## graphify

This project has a knowledge graph at graphify-out/ with god nodes, community structure, and cross-file relationships.

Rules:
- For codebase questions, first run `graphify query "<question>"` when graphify-out/graph.json exists. Use `graphify path "<A>" "<B>"` for relationships and `graphify explain "<concept>"` for focused concepts. These return a scoped subgraph, usually much smaller than GRAPH_REPORT.md or raw grep output.
- If graphify-out/wiki/index.md exists, use it for broad navigation instead of raw source browsing.
- Read graphify-out/GRAPH_REPORT.md only for broad architecture review or when query/path/explain do not surface enough context.
- After modifying code, run `graphify update .` to keep the graph current (AST-only, no API cost).
