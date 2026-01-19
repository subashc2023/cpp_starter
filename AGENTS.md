# GGEngine Context

## Project Overview
**GGEngine** is a minimal C++ game engine designed with a modular architecture. It consists of a core Engine library and client applications (`Sandbox` and `Editor`) that link against it.

- **Type:** C++ Game Engine
- **Architecture:** 
    - **Engine:** Core shared library (DLL) containing the application lifecycle, logging, and systems.
    - **Sandbox:** A testing application for engine features.
    - **Editor:** A toolset application built on top of the engine.
- **Language:** C++ (Standard inferred from CMake config, likely C++17 or newer).
- **Build System:** CMake (3.20+) using Ninja generator.

## Building and Running

### Prerequisites
- CMake (3.20+)
- LLVM/Clang
- Ninja
- MSYS2 MinGW-w64 (for Windows)

### Build Commands
The project uses helper scripts in the `scripts/` directory to manage CMake presets.

**Windows:**
```powershell
# Build everything (Engine, Sandbox, Editor)
.\scripts\build-all.bat <debug|release|dist>

# Build specific targets
.\scripts\build-engine.bat <debug|release|dist>
.\scripts\build-sandbox.bat <debug|release|dist>
.\scripts\build-editor.bat <debug|release|dist>

# Clean and Build
.\scripts\clean_and_build.bat <debug|release|dist>
```

**Linux/macOS:**
```bash
./scripts/build-all <debug|release|dist>
```

### Build Configurations
- **Debug:** (Default) Builds with symbols, logging enabled, and optimizations off. Engine is built as a shared library (DLL).
- **Release:** Optimizations enabled, but logging is still present. Engine is built as a static library (no DLL).
- **Dist:** (Distribution) Fully optimized, logging macros are stripped (`GG_DIST` defined), and the Engine is static. Use this for shipping games.

### Running Applications
Output binaries are located in the `bin/` directory, organized by configuration.

```powershell
# Example for Debug build on Windows
.\bin\Debug-x64\Sandbox\Sandbox.exe
.\bin\Debug-x64\Editor\Editor.exe
```

## Development Conventions

### Code Structure
- **Engine Source:** `Engine/src/GGEngine/`
- **Client Source:** `Sandbox/src/` and `Editor/src/`
- **Dependencies:** `Vendor/` (currently contains `spdlog`).

### Key Mechanisms
- **Entry Point:** The `main` function is defined in `Engine/src/GGEngine/EntryPoint.h`. Clients do not write `main`; instead, they must implement `GGEngine::CreateApplication()` to return a new instance of their derived `Application` class.
- **Logging:** The engine uses a wrapper around `spdlog`.
    - **Engine Log Macros:** `GG_CORE_TRACE`, `GG_CORE_INFO`, `GG_CORE_WARN`, `GG_CORE_ERROR`, `GG_CORE_FATAL`.
    - **Client Log Macros:** `GG_TRACE`, `GG_INFO`, `GG_WARN`, `GG_ERROR`, `GG_FATAL`.
- **API Visibility:** The `GG_API` macro handles DLL export/import (Windows).
- **Platform:** Currently targets `GG_PLATFORM_WINDOWS`.

### Build & Configuration
- **Presets:** CMake presets are used to configure the build (handled by `scripts/resolve-preset.bat`).
- **Output:** Binaries are output to `bin/` and intermediate build files to `build/`.
