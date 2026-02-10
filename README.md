# ParusEngine

![alt text](https://github.com/user-attachments/assets/e9e6cb6e-4794-4cc9-8258-982b010c07a9)


---

## Table of Contents

- [Introduction](#introduction)
- [Acknowledgements](#acknowledgements)
- [Features](#features)
- [Roadmap](#roadmap)
- [Platform Support](#platform-support)
- [Requirements](#requirements)
- [Building](#building)

---

## Introduction

ParusEngine is a lightweight real-time rendering engine written in C++ and built on top of the Vulkan API.  
The long-term vision of the engine is to support dynamic loading of models, scenes, and complete games directly from the web using a custom binary format.

The project is currently in active development and focuses on rendering architecture, multithreading, and physically based rendering.

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

- Modular C++ architecture  
- ImGui integration for debugging and development tools  
- Platform abstraction layer prepared for future cross-platform support
  
---

## Roadmap

### Current Work

- Refactoring and finalizing the Vulkan rendering module  
- Completing cubemap implementation  
- Improving renderer abstractions and internal structure  

### Planned Features

- Custom binary file format for storing:
  - Meshes  
  - Textures  
  - Scenes  
  - Runtime logic  
- Runtime loader for dynamically downloaded scene files  
- Ability to download and run scenes or games from a server  
- Expanded asset pipeline  
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

- Microsoft Visual Studio (recommended version 2022)  
- Vulkan SDK (latest version provided by LunarG)  

No additional third-party dependencies are required at this stage.

---

## Building

1. Install the Vulkan SDK  
2. Clone the repository:
```bash
git clone https://github.com/bogdanputintsev/ParusEngine
```
3. Open ParusEngine.sln in Visual Studio
4. Select Debug or Release configuration
5. Build and run the project

Shader compilation is handled via the provided `compile_shaders.bat` script.

