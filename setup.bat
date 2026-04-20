@echo off
setlocal enabledelayedexpansion

echo.
echo ------------------------------------------------------------------
echo       WhisperGlass - Dependency Download ^& Environment Setup      
echo ------------------------------------------------------------------
echo.

:: --- Check Tools -------------------------------------------------------------
where git >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Git not found. Please install Git for Windows: https://git-scm.com
    pause & exit /b 1
)

echo [OK] Git is ready.

where cmake >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake not found. Please install CMake: https://cmake.org/download
    pause & exit /b 1
)

echo [OK] CMake is ready.

echo Skipping curl check since Windows 10 includes it by default. If you encounter issues, please ensure curl is available in your PATH or download it from: https://curl.se/windows/
echo [OK] curl is ready.

echo [OK] Tool check passed.
echo.

:: --- Create Directories ------------------------------------------------------
if not exist "third_party"   mkdir third_party
if not exist "bin"           mkdir bin


:: ==============================================================================
::  1. Download Dear ImGui
:: ==============================================================================
echo [1/3] Cloning Dear ImGui...
if exist "third_party\imgui\.git" (
    echo       Directory exists. Skipping...
) else (
    git clone --depth=1 https://github.com/ocornut/imgui.git third_party/imgui
    if errorlevel 1 ( echo [ERROR] Failed to clone ImGui & pause & exit /b 1 )
)
echo [OK] ImGui is ready.
echo.

:: ==============================================================================
::  2. Download GLFW Pre-compiled Binaries (Windows 64-bit)
:: ==============================================================================
echo [2/3] Downloading GLFW Pre-compiled Binaries...
if exist "third_party\glfw\include\GLFW\glfw3.h" (
    echo       Files exist. Skipping...
) else (
    set GLFW_VER=3.4
    set GLFW_ZIP=glfw-3.4.bin.WIN64.zip
    set GLFW_URL=https://github.com/glfw/glfw/releases/download/3.4/glfw-3.4.bin.WIN64.zip

    echo       Downloading %GLFW_ZIP%...
    curl -L -o third_party\%GLFW_ZIP% %GLFW_URL%
    if errorlevel 1 ( echo [ERROR] GLFW download failed & pause & exit /b 1 )

    echo       Extracting GLFW...
    powershell -Command "Expand-Archive -Path 'third_party\%GLFW_ZIP%' -DestinationPath 'third_party\glfw_tmp' -Force"
    
    if exist "third_party\glfw" rmdir /s /q third_party\glfw
    move "third_party\glfw_tmp\glfw-%GLFW_VER%.bin.WIN64" "third_party\glfw"
    rmdir /s /q third_party\glfw_tmp
    del third_party\%GLFW_ZIP%

    if exist "third_party\glfw\lib-vc2022" (
        echo       [OK] Found lib-vc2022 folder.
    ) else if exist "third_party\glfw\lib-vc2019" (
        echo       [OK] Found lib-vc2019. Creating compatibility directory...
        xcopy /e /i "third_party\glfw\lib-vc2019" "third_party\glfw\lib-vc2022" >nul
    )
)
echo [OK] GLFW is ready.
echo.

echo [3/3] Checking  dependencies...
echo  Directory Status:
if exist "third_party\imgui"       echo   - ImGui: OK
if exist "third_party\glfw"        echo   - GLFW: OK
echo.

:: --- Final Report ------------------------------------------------------------
echo ------------------------------------------------------------------
echo  [SUCCESS] All dependencies are ready. Run build.bat to compile.
echo ------------------------------------------------------------------
echo.

pause