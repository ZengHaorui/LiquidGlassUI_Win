@echo off
setlocal enabledelayedexpansion

echo.
echo ------------------------------------------------------------------
echo "            WhisperGlass - Build & Compilation Script"
echo ------------------------------------------------------------------
echo.

:: --- Check Visual Studio -----------------------------------------------------
:: Attempting to locate vcvarsall.bat for VS2022 / VS2019
set VCVARS="C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"

goto :run_cmake

:: --- CMake Configuration & Build ---------------------------------------------
:run_cmake
if not exist "build" mkdir build >nul 2>&1
cd build >nul 2>&1

echo.
echo [CMake] Configuring project (Release x64)...

cmake -S ".." -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -DWHISPER_AVX=ON -DWHISPER_AVX2=ON -DWHISPER_FMA=ON >nul 2>&1

if errorlevel 1 (
    echo [X]
    echo [ERROR] CMake configuration failed!
    echo         Common reasons:
    echo           1. VS 2022 not installed.
    echo           2. Dependencies not downloaded.
    echo           3. CMake version is too old.
    cd ..
    pause
    exit /b 1
)

echo [CMake] OKAY.

echo [CMake] Building project (Release)...
cmake --build . --config Release --parallel

if errorlevel 1 (
    echo [ERROR] Build failed!
    cd ..
    pause
    exit /b 1
)

cd ..

:: --- Copy Runtime Files ------------------------------------------------------
echo.
echo [OK] Build successful! Copying runtime files...

:: Copy models directory to output
if exist "models" (
    xcopy /e /i /y "models" "bin\models" >nul 2>&1
    echo [OK] Models directory copied to bin\models\
)

:: Copy GLFW DLLs if they exist
for %%F in (third_party\glfw\lib-vc2022\glfw3.dll third_party\glfw\bin\*.dll) do (
    if exist "%%F" copy /y "%%F" "bin\" >nul 2>&1
)

echo.
echo ------------------------------------------------------------------
echo  [SUCCESS] Build Complete!
echo.
echo  Executable location: bin\WhisperGlass.exe
echo.
echo  Usage:
echo    - Double-click bin\WhisperGlass.exe
echo    - Or specify model: bin\WhisperGlass.exe models\ggml-base-q5_1.bin
echo ------------------------------------------------------------------
echo.

