@echo off

REM Set the script to exit on error
setlocal EnableExtensions EnableDelayedExpansion

REM Change the current directory to the directory of the script
pushd "%~dp0"

REM Define the paths for convenience
set PROJECT_DIR=TesseraEngine
set SHADERS_DIR=%PROJECT_DIR%\source\shaders
set VERTEX_SHADER=%SHADERS_DIR%\shader.vert
set FRAGMENT_SHADER=%SHADERS_DIR%\shader.frag
set BUILD_DIR=build

REM Check if vertex shader file exists
if not exist "%VERTEX_SHADER%" (
    echo [ERROR] Shader file "%VERTEX_SHADER%" does not exist. Exiting.
    pause
    exit /b 1
)

REM Check if fragment shader file exists
if not exist "%FRAGMENT_SHADER%" (
    echo [ERROR] Shader file "%FRAGMENT_SHADER%" does not exist. Exiting.
    pause
    exit /b 1
)

REM Create the build directory if it doesn't exist
if not exist "%BUILD_DIR%" (
    echo [INFO] Creating build directory "%BUILD_DIR%"...
    mkdir "%BUILD_DIR%"
)

REM Create the bin directory if it doesn't exist
if not exist "%BUILD_DIR%\bin" (
    echo [INFO] Creating bin directory "%BUILD_DIR%\bin"...
    mkdir "%BUILD_DIR%\bin"
)

REM Compile the shader using glslc in build folder
echo [INFO] Compiling shader file "%VERTEX_SHADER%"...
"%VULKAN_SDK%\Bin\glslc.exe" "%VERTEX_SHADER%" -o "%BUILD_DIR%\bin\vert.spv"
echo [INFO] Compiling shader file "%FRAGMENT_SHADER%"...
"%VULKAN_SDK%\Bin\glslc.exe" "%FRAGMENT_SHADER%" -o "%BUILD_DIR%\bin\frag.spv"

REM Check if the compilation was successful
if %errorlevel% neq 0 (
    echo [ERROR] Failed to compile the shader. Exiting.
    pause
    exit /b 1
)

echo [INFO] Shaders compiled successfully to "%BUILD_DIR%\bin".
REM Copy compiled files to the destination directory

if not exist "%PROJECT_DIR%\bin" (
    echo [INFO] Creating bin directory "%PROJECT_DIR%\bin"...
    mkdir "%PROJECT_DIR%\bin"
)

echo [INFO] Copying files from "%BUILD_DIR%\bin" to "%PROJECT_DIR%\bin"...
xcopy /E /I /Q /Y "%BUILD_DIR%\bin\*" "%PROJECT_DIR%\bin"

if %errorlevel% neq 0 (
    echo [ERROR] Failed to copy files to "%PROJECT_DIR%\bin". Exiting.
    exit /b 1
)

echo [INFO] All files successfully copied to "%PROJECT_DIR%\bin".
exit /b 0
