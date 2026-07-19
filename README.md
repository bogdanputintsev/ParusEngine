# ParusEngine

[![Automation Tests](https://github.com/bogdanputintsev/ParusEngine/actions/workflows/ci.yml/badge.svg)](https://github.com/bogdanputintsev/ParusEngine/actions/workflows/ci.yml)

![alt text](https://github.com/user-attachments/assets/e9e6cb6e-4794-4cc9-8258-982b010c07a9)


---

## Table of Contents

- [Introduction](#introduction)
- [Acknowledgements](#acknowledgements)
- [Features](#features)
- [Architecture](#architecture)
- [Roadmap](#roadmap)
- [Platform Support](#platform-support)
- [Requirements](#requirements)
- [Building](#building)
- [Testing](#testing)

---

## Introduction

ParusEngine is a lightweight real-time rendering engine written in C++20 and built on top of the Vulkan API.  
The long-term vision of the engine is to support dynamic loading of models, scenes, and complete games directly from the web using a custom binary format, driven by creator-side generative AI.

The project is currently in active development and focuses on authoring tools, scene serialization, and physically based rendering.

---

## Acknowledgements

This project is dedicated and named after my wife, Xenia, who always supports, inspires, and motivates me on my path.

---

## Features

### Rendering

- Vulkan-based rendering backend  
- Physically Based Rendering (PBR)  
- Support for PBR texture sets:
  - Albedo  
  - Normal  
  - Metallic  
  - Roughness  
  - Ambient Occlusion  
  - Emissive  
- Early-stage cubemap implementation  

### Asset System

- Multithreaded OBJ model loading  
- Texture loading system  
- Resource lifetime management  

### Engine Systems

- Service-locator dependency injection (`Services`)  
- Type-safe, `std::any`-backed event system  
- In-engine console with trie-based tab-completion  
- Custom binary serialization for meshes, textures, and scenes (`.pmesh` / `.ptex` / `.pworld`)  
- Thread pool for async work  
- ImGui integration for debugging and development tools  
- Platform abstraction layer prepared for future cross-platform support

---

## Architecture

- **Application lifecycle** — `init()` (register services, load config, create window, init Vulkan + ImGui) → `loop()` (delta time, world tick, platform messages, render frame) → `clean()` (ordered shutdown).
- **Renderer** — multi-pass Vulkan pipeline: shadow pass → depth pre-pass → SSAO → SSAO blur → main pass. Every Vulkan resource (instance, device, swap chain, pipelines, images, descriptors) is constructed via a dedicated builder/factory class.
- **World** — a `Storage` container of entities, mesh instances, and lights, plus a spectator camera. This is what the renderer draws from and what serialization reads/writes.
- **Serialization** — a custom binary format (foundation for loading scenes from the web) with `save` / `import` console commands.

---

## Roadmap

### Current Work

- Console command API for full scene authoring: spawn objects, move/transform them, and script game logic entirely through console commands  
- Improving renderer abstractions and internal structure  

### Planned Features

- Creator-side generative AI: describe a scene in natural language, engine composes it from a local asset catalog by driving the same console command API  
- ParusStore backend for downloading assets and scenes over HTTP  
- Runtime loader for dynamically downloaded scene files  
- Minimal gameplay primitives (character controller, collision)  
- Cross-platform build support  

---

## Platform Support

| Platform | Status |
|--------|--------|
| Windows | Supported |
| Linux | Planned |
| macOS | Planned |

At the moment, the engine builds and runs on Windows using Microsoft Visual Studio.  
The internal architecture is designed to support additional operating systems in the future.

---

## Requirements

To build ParusEngine, you need:

- Windows with an x64 MSVC toolchain (Visual Studio 2022 Build Tools or full IDE), providing `cl.exe` on `PATH`  
- CMake >= 3.20 with the Ninja generator  
- Vulkan SDK (latest version provided by LunarG), with `VULKAN_SDK` set in the environment  

Third-party libraries (ImGui, stb_image, tiny_obj_loader) are bundled in `source/third-party/`. GoogleTest is fetched automatically by CMake and only needed for the test target.

---

## Building

1. Install the Vulkan SDK and set the `VULKAN_SDK` environment variable
2. Clone the repository:
```bash
git clone https://github.com/bogdanputintsev/ParusEngine
```
3. Open an x64 MSVC developer shell, then configure and build:
```bash
cmake --preset debug          # or: release
cmake --build --preset debug
```
4. Run `ParusEngine.exe` from `build/debug/bin/` (assets and shaders are copied there automatically as a post-build step)

Shader source lives in `source/shaders/`; run `compile_shaders.sh` to recompile GLSL to SPIR-V via `glslc`.

---

## Testing

Tests use GoogleTest and are discovered automatically:

```bash
ctest --preset debug
```

CI (GitHub Actions) builds and runs the full test suite on `windows-latest` for every push/PR to `master`.

