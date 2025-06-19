@echo off

REM Set the script to exit on error
setlocal EnableExtensions EnableDelayedExpansion

REM Change the current directory to the directory of the script
pushd "%~dp0"

REM Base directory 
set PROJECT_DIR=TesseraEngine

REM Shaders source code
set SOURCE_SHADERS_DIR=TesseraEngine\source\shaders
set SOURCE_VERTEX_SHADER=%SOURCE_SHADERS_DIR%\shader.vert
set SOURCE_FRAGMENT_SHADER=%SOURCE_SHADERS_DIR%\shader.frag

REM Output shaders directory
set SHADERS_OUTPUT_DIR=TesseraEngine\bin\shaders
set OUTPUT_BUILD_DIR=build

if not exist "%VULKAN_SDK%" (
    echo [ERROR] VULKAN_SDK path does not exist. Exiting.
    pause
    exit /b 1
)

if not exist "%SOURCE_VERTEX_SHADER%" (
    echo [ERROR] Shader file "%SOURCE_VERTEX_SHADER%" does not exist. Exiting.
    pause
    exit /b 1
)

if not exist "%SOURCE_FRAGMENT_SHADER%" (
    echo [ERROR] Shader file "%SOURCE_FRAGMENT_SHADER%" does not exist. Exiting.
    pause
    exit /b 1
)

if not exist "%SHADERS_OUTPUT_DIR%" (
    echo [INFO] Creating directory for shaders output "%SHADERS_OUTPUT_DIR%"...
    mkdir "%SHADERS_OUTPUT_DIR%"
)

REM Compile the shader using glslc in build folder
echo [INFO] Compiling shader file "%SOURCE_VERTEX_SHADER%"...
"%VULKAN_SDK%\Bin\glslc.exe" "%SOURCE_VERTEX_SHADER%" -o "%SHADERS_OUTPUT_DIR%\vert.spv"
echo [INFO] Compiling shader file "%SOURCE_FRAGMENT_SHADER%"...
"%VULKAN_SDK%\Bin\glslc.exe" "%SOURCE_FRAGMENT_SHADER%" -o "%SHADERS_OUTPUT_DIR%\frag.spv"

REM Check if the compilation was successful
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile the shader. Exiting.
    pause
    exit /b 1
)

echo [INFO] Shaders compiled successfully to "%SHADERS_OUTPUT_DIR%".