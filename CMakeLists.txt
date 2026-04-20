cmake_minimum_required(VERSION 3.18)
project(WhisperGlass CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

# ── 编译选项 (保留以支持后续可能的性能优化) ───────────────────────────────────
option(WHISPER_AVX    "Enable AVX"   ON)
option(WHISPER_AVX2   "Enable AVX2"  ON)
option(WHISPER_FMA    "Enable FMA"   ON)

# ── 编译器标志 ───────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(
        /O2 /arch:AVX2 /W3 /MP /utf-8
        /wd4244 /wd4267 /wd4819
    )
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
else()
    add_compile_options(-O3 -march=native -Wall -Wextra)
endif()

# ── 依赖路径定义 ──────────────────────────────────────────────────────────
set(WHISPER_SRC_DIR ${CMAKE_SOURCE_DIR}/third_party/whisper.cpp)
set(IMGUI_DIR       ${CMAKE_SOURCE_DIR}/third_party/imgui)

# ── whisper.cpp 源码逻辑 (保留作为依赖，但不用于 WhisperGlass) ──────────────────
if(EXISTS ${WHISPER_SRC_DIR}/src/whisper.cpp)
    set(WHISPER_INCLUDE_DIRS
        ${WHISPER_SRC_DIR}/include
        ${WHISPER_SRC_DIR}/ggml/include
        ${WHISPER_SRC_DIR}/ggml/src
        ${WHISPER_SRC_DIR}/ggml/src/ggml-cpu
    )
    # 此处省略了原有的 GLOB 源码搜索逻辑以保持文件整洁，
    # 如果你的 demo 之后需要链接 whisper，可以重新加回。
else()
    set(WHISPER_INCLUDE_DIRS ${WHISPER_SRC_DIR})
endif()

# ── Dear ImGui 源码 ─────────────────────────────────────────────────────────
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# ── 寻找库依赖 ──────────────────────────────────────────────────────────────
find_library(GLFW_LIB
    NAMES glfw3 glfw3dll
    PATHS
        ${CMAKE_SOURCE_DIR}/third_party/glfw/lib-vc2022
        ${CMAKE_SOURCE_DIR}/third_party/glfw/lib-vc2019
        ${CMAKE_SOURCE_DIR}/third_party/glfw/lib
    NO_DEFAULT_PATH
)
if(NOT GLFW_LIB)
    message(FATAL_ERROR "GLFW library not found. Run setup.bat first.")
endif()

find_package(OpenGL REQUIRED)

# ════════════════════════════════════════════════════════════════════════════
#  Glass Node Demo (当前唯一编译目标)
# ════════════════════════════════════════════════════════════════════════════
set(DEMO_SOURCES
    src/demo_main.cpp
)

add_executable(GlassNodeDemo
    ${DEMO_SOURCES}
    ${IMGUI_SOURCES}
)

# ── 包含目录 ─────────────────────────────────────────────────────────────
target_include_directories(GlassNodeDemo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${WHISPER_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/third_party/glfw/include
)

# ── 编译定义 ──────────────────────────────────────────────────────────────
target_compile_definitions(GlassNodeDemo PRIVATE
    IMGUI_IMPL_OPENGL_LOADER_GLFW
    _USE_MATH_DEFINES
    NOMINMAX
)

# ── 链接库 ────────────────────────────────────────────────────────────────
target_link_libraries(GlassNodeDemo PRIVATE
    ${GLFW_LIB}
    opengl32
    dwmapi
    winmm
    kernel32
    user32
    gdi32
    shell32
    ${OPENGL_LIBRARIES}
)

# ── 输出设置 ──────────────────────────────────────────────────────────────
set_target_properties(GlassNodeDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

# ── 状态信息 ──────────────────────────────────────────────────────────────
message(STATUS "=======================================================")
message(STATUS " GlassNodeDemo Build Configured")
message(STATUS " MSVC:        ${MSVC}")
message(STATUS " GLFW Lib:    ${GLFW_LIB}")
message(STATUS " Output Dir:  ${CMAKE_SOURCE_DIR}/bin")
message(STATUS "=======================================================")