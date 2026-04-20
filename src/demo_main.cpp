// ================================================================
//  demo_main.cpp (完整补全版)
// ================================================================

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <GL/gl.h>
#include <vector>
#include <cmath>
#include <iostream>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// --- OpenGL 扩展加载定义 ---
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_BGRA 0x80E1
#define GL_TEXTURE0 0x84C0
#define GL_TEXTURE1 0x84C1
#define GL_TEXTURE2 0x84C2

#ifndef GLFW_ALWAYS_ON_TOP
#define GLFW_ALWAYS_ON_TOP 0x00020002
#endif

// --- 补充编译和链接状态相关的宏 ---
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_SHADING_LANGUAGE_VERSION 0x8B8C

typedef ptrdiff_t GLsizeiptr;
typedef char GLchar;

#define DECL_GL(ret, name, ...)                   \
    typedef ret(WINAPI *PFN_##name)(__VA_ARGS__); \
    extern PFN_##name name

// 声明扩展函数
DECL_GL(GLuint, glCreateShader, GLenum);
DECL_GL(void, glShaderSource, GLuint, GLsizei, const GLchar *const *, const GLint *);
DECL_GL(void, glCompileShader, GLuint);
DECL_GL(GLuint, glCreateProgram, void);
DECL_GL(void, glAttachShader, GLuint, GLuint);
DECL_GL(void, glLinkProgram, GLuint);
DECL_GL(void, glUseProgram, GLuint);
DECL_GL(void, glGenVertexArrays, GLsizei, GLuint *);
DECL_GL(void, glGenBuffers, GLsizei, GLuint *);
DECL_GL(void, glBindVertexArray, GLuint);
DECL_GL(void, glBindBuffer, GLenum, GLuint);
DECL_GL(void, glBufferData, GLenum, GLsizeiptr, const void *, GLenum);
DECL_GL(void, glEnableVertexAttribArray, GLuint);
DECL_GL(void, glVertexAttribPointer, GLuint, GLint, GLenum, GLboolean, GLsizei, const void *);
DECL_GL(GLint, glGetUniformLocation, GLuint, const GLchar *);
DECL_GL(void, glUniform1f, GLint, GLfloat);
DECL_GL(void, glUniform1i, GLint, GLint);
DECL_GL(void, glUniform2f, GLint, GLfloat, GLfloat);
DECL_GL(void, glUniform4f, GLint, GLfloat, GLfloat, GLfloat, GLfloat);
DECL_GL(void, glActiveTexture, GLenum);

DECL_GL(void, glGetShaderiv, GLuint, GLenum, GLint *);
DECL_GL(void, glGetShaderInfoLog, GLuint, GLsizei, GLsizei *, GLchar *);
DECL_GL(void, glGetProgramiv, GLuint, GLenum, GLint *);
DECL_GL(void, glGetProgramInfoLog, GLuint, GLsizei, GLsizei *, GLchar *);


DECL_GL(void, glGetShaderiv, GLuint, GLenum, GLint *);
DECL_GL(void, glGetProgramiv, GLuint, GLenum, GLint *);
// 定义函数指针
#define DEF_GL(name) PFN_##name name = nullptr
DEF_GL(glCreateShader);
DEF_GL(glShaderSource);
DEF_GL(glCompileShader);
DEF_GL(glCreateProgram);
DEF_GL(glAttachShader);
DEF_GL(glLinkProgram);
DEF_GL(glUseProgram);
DEF_GL(glGenVertexArrays);
DEF_GL(glGenBuffers);
DEF_GL(glBindVertexArray);
DEF_GL(glBindBuffer);
DEF_GL(glBufferData);
DEF_GL(glEnableVertexAttribArray);
DEF_GL(glVertexAttribPointer);
DEF_GL(glGetUniformLocation);
DEF_GL(glUniform1f);
DEF_GL(glUniform1i);
DEF_GL(glUniform2f);
DEF_GL(glUniform4f);
DEF_GL(glActiveTexture);

// 在 DEF_GL 区块末尾追加
DEF_GL(glGetShaderiv);
DEF_GL(glGetShaderInfoLog);
DEF_GL(glGetProgramiv);
DEF_GL(glGetProgramInfoLog);




// 加载扩展函数的实现
void LoadModernGL()
{
    auto load = [](const char *name)
    {
        void *p = (void *)wglGetProcAddress(name);
        if (!p || p == (void *)0x1 || p == (void *)0x2 || p == (void *)0x3 || p == (void *)-1)
        {
            HMODULE module = GetModuleHandleA("opengl32.dll");
            p = (void *)GetProcAddress(module, name);
        }
        return p;
    };

    glCreateShader = (PFN_glCreateShader)load("glCreateShader");
    glShaderSource = (PFN_glShaderSource)load("glShaderSource");
    glCompileShader = (PFN_glCompileShader)load("glCompileShader");
    glCreateProgram = (PFN_glCreateProgram)load("glCreateProgram");
    glAttachShader = (PFN_glAttachShader)load("glAttachShader");
    glLinkProgram = (PFN_glLinkProgram)load("glLinkProgram");
    glUseProgram = (PFN_glUseProgram)load("glUseProgram");
    glGenVertexArrays = (PFN_glGenVertexArrays)load("glGenVertexArrays");
    glGenBuffers = (PFN_glGenBuffers)load("glGenBuffers");
    glBindVertexArray = (PFN_glBindVertexArray)load("glBindVertexArray");
    glBindBuffer = (PFN_glBindBuffer)load("glBindBuffer");
    glBufferData = (PFN_glBufferData)load("glBufferData");
    glEnableVertexAttribArray = (PFN_glEnableVertexAttribArray)load("glEnableVertexAttribArray");
    glVertexAttribPointer = (PFN_glVertexAttribPointer)load("glVertexAttribPointer");
    glGetUniformLocation = (PFN_glGetUniformLocation)load("glGetUniformLocation");
    glUniform1f = (PFN_glUniform1f)load("glUniform1f");
    glUniform1i = (PFN_glUniform1i)load("glUniform1i");
    glUniform2f = (PFN_glUniform2f)load("glUniform2f");
    glUniform4f = (PFN_glUniform4f)load("glUniform4f");
    glActiveTexture = (PFN_glActiveTexture)load("glActiveTexture");

    // 在 LoadModernGL() 末尾追加
    glGetShaderiv = (PFN_glGetShaderiv)load("glGetShaderiv");
    glGetShaderInfoLog = (PFN_glGetShaderInfoLog)load("glGetShaderInfoLog");
    glGetProgramiv = (PFN_glGetProgramiv)load("glGetProgramiv");
    glGetProgramInfoLog = (PFN_glGetProgramInfoLog)load("glGetProgramInfoLog");
}

#include "LiquidGlassGround.h"
namespace LGG = LiquidGlassGround; // 定义别名

// ── 屏幕捕获实现 ─────────────────────────────────────
static void CaptureScreen(HWND hwnd, GLuint tex, float &lumaOut)
{
    RECT wr;
    GetWindowRect(hwnd, &wr);
    int w = wr.right - wr.left, h = wr.bottom - wr.top;
    if (w <= 0 || h <= 0)
        return;

    HDC sdc = GetDC(NULL); // 屏幕 DC
    HDC mdc = CreateCompatibleDC(sdc);
    HBITMAP hbm = CreateCompatibleBitmap(sdc, w, h);
    HBITMAP old = (HBITMAP)SelectObject(mdc, hbm);

    // 修复点 1: 添加 CAPTUREBLT。这对于处理半透明窗口至关重要。
    // 同时确保坐标正确，wr.left/top 是相对于屏幕的。
    BitBlt(mdc, 0, 0, w, h, sdc, wr.left, wr.top, SRCCOPY | CAPTUREBLT);

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = w;
    bmi.bmiHeader.biHeight = -h;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<uint32_t> px(w * h);
    GetDIBits(mdc, hbm, 0, h, px.data(), &bmi, DIB_RGB_COLORS);

    // 修复点 2: 亮度统计。如果抓到全是 0，强制 luma 为 0.5 避免高光变黑。
    double sum = 0;
    int count = 0;
    for (int i = 0; i < w * h; i += 64)
    { // 步长加大点没关系
        uint32_t c = px[i];
        float r = ((c >> 16) & 0xFF) / 255.f;
        float g = ((c >> 8) & 0xFF) / 255.f;
        float b = (c & 0xFF) / 255.f;
        sum += (0.299f * r + 0.587f * g + 0.114f * b);
        count++;
    }
    lumaOut = (count > 0 && sum > 0.01) ? (float)(sum / count) : 0.2f;

    glBindTexture(GL_TEXTURE_2D, tex);
    // 修复点 3: 确保纹理上传参数正确
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, px.data());

    SelectObject(mdc, old);
    DeleteObject(hbm);
    DeleteDC(mdc);
    ReleaseDC(NULL, sdc);
}

// ================================================================
//  Demo 类
// ================================================================
struct Demo
{

    bool firstFrame = true; // ✅ 新增

    LGG::RoundRect card;
    LGG::Circle bubble;
    LGG::Text label;
    LGG::Icon icon;
    float time = 0.f;
    GLuint iconTexture = 0;

    void Init()
    {
        card.id = "demo_card";
        card.pos = {320.f, 200.f, 80.f, 60.f, 18.f};
        card.mat = LGG::Preset::Liquid();
        card.mat.shadow.opacity = 0.4f;
        card.mat.shadow.direction = {0.f, 0.f};
        card.mat.shadow.size=0.0f;


        card.mat.blur.centerBlur = 0.0f;
        card.mat.blur.edgeBlur = 100.0f;
        card.mat.blur.startMargin = 10.0f;
        card.mat.refraction.dispersionCoeff = 0.2f; // 轻微色散

        bubble.id = "demo_bubble";
        bubble.pos = {80.f, 80.f, 320.f, 50.f, 0.f};
        bubble.mat = LGG::Preset::Liquid();
        bubble.mat.shadow.size = 0.0f;

    
        // 设置位置：宽80, 高80, X=320, Y=50
        bubble.mat = LGG::Preset::Liquid();


        label.text = "ABCDE";
        label.fontSize = 128.f;
        label.pos = {200.f, 100.f, 140.f, 120.f, 12.f};
        label.mat = LGG::Preset::Liquid();

        // --- 添加调试颜色 ---
        label.mat.color.baseColor = {0.5f, 0.8f, 1.0f, 0.5f}; // 蓝色半透明
        label.mat.color.tintStrength = 0.0f;                  // 染色强度
        label.mat.refraction.dispersionCoeff = 0.0f;              // 明显色散
        label.mat.blur.centerBlur = 1.0f;
        label.mat.refraction.flatTopRange=0;
        label.mat.refraction.refractIndex=1.5f;
        label.mat.light.emitColor=ImVec4(0.0f,0.0f,0.0f,0.0f);
        //label.BakeNormalMap(); // 必须在 ImGui 字体加载后

        bubble.on_hover = [&]
        {
            bubble.mat.light.intensity = 0.8f;
            //bubble.mat.light.emitColor = {0.4f, 0.7f, 1.0f, 1.0f};

            bubble.setTargetPosition({120.f, 120.f, 320.f, 50.f, 0.f}, 10);

        };

        bubble.on_unhover = [&]
        {
            bubble.mat.light.intensity = 0.0f;
            //bubble.mat.light.emitColor = {0.2f, 0.5f, 0.8f, 1.0f};
            bubble.setTargetPosition({80.f, 80.f, 320.f, 50.f, 0.f}, 10);
        };

    }

    void Update(float dt)
    {
        time += dt;
        //bubble.pos.x = 320.f + std::sin(time) * 30.f;
        //bubble.pos.y = 50.f + std::cos(time * 1.5f) * 15.f;
        //bubble.mat.light.intensity = std::max(0.f, bubble.mat.light.intensity - dt * 2.0f);
    }

    void Render()
    {
        ImGui::SetNextWindowPos({0, 0});
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);

        if (firstFrame)
        {
            label.BakeNormalMap();
            firstFrame = false;
        }

        if (ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground ))
        {
            card.Render();
            bubble.Render();
            //label.Render();
        }
        ImGui::End();
    }
};

// ── 窗口区域构建（AA + 阴影扩展）──────────────────────────────────

static void UpdateWindowRegion(HWND hwnd)
{
    HRGN hTotal = NULL;

    for (const auto &cmd : LGG::s_Cmds)
    {
        const auto &p = cmd.pos;
        const auto &sh = cmd.mat.shadow;

        // 基础扩展 2px：为 shader smoothstep AA 过渡像素留空间
        int expand = 0;

        // 阴影扩展：让阴影像素也落在 Region 内，不被 GDI 截断
        /*if (sh.opacity > 0.01f)
        {
            int shExt = (int)(std::abs(sh.direction.x) + std::abs(sh.direction.y) + sh.size + sh.blur * 0.5f) + 2;
            expand = std::max(expand, shExt);
        }*/

        int x1 = (int)(p.x) - expand;
        int y1 = (int)(p.y) - expand;
        int x2 = (int)(p.x + p.width) + expand + 1;
        int y2 = (int)(p.y + p.height) + expand + 1;

        HRGN hSub;
        if (cmd.kind == LGG::ShapeKind::Circle)
        {
            hSub = CreateEllipticRgn(x1, y1, x2, y2);
        }
        else
        {
            int cr = ((int)p.cornerRadius + expand) * 2;
            hSub = CreateRoundRectRgn(x1, y1, x2, y2, cr, cr);
        }

        if (!hTotal)
            hTotal = hSub;
        else
        {
            CombineRgn(hTotal, hTotal, hSub, RGN_OR);
            DeleteObject(hSub);
        }
    }

    if (hTotal)
        SetWindowRgn(hwnd, hTotal, TRUE);
}

int main()
{
    // 初始化 GLFW
    if (!glfwInit()) return 1;

    // 设置窗口属性
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE); // 无边框窗口
    glfwWindowHint(GLFW_ALWAYS_ON_TOP, GLFW_TRUE); // 窗口始终置顶

    // 创建窗口
    GLFWwindow *win = glfwCreateWindow(800, 600, "LiquidGlass", nullptr, nullptr);
    if (!win) return -1;
    glfwMakeContextCurrent(win);
    LoadModernGL();

    // 获取窗口句柄并设置显示属性
    HWND hwnd = glfwGetWin32Window(win);

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    // 防止捕获到自身（造成递归视觉反馈）
    SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);

    // 隐藏控制台窗口
    HWND console = GetConsoleWindow();
    if (console) ShowWindow(console, SW_HIDE); // 隐藏控制台窗口


    // 初始化 ImGui
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    LGG::Init();

    GLuint screenTex;

    // 创建屏幕纹理
    glGenTextures(1, &screenTex);
    glBindTexture(GL_TEXTURE_2D, screenTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // ✅ 新增
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // ✅ 新增
    glBindTexture(GL_TEXTURE_2D, 0);

    Demo demo;
    demo.Init();

    bool isDragging = false;
    double dragOffsetX = 0, dragOffsetY = 0;

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        // --- 新增拖动逻辑 ---
        // 只有当鼠标点击了窗口，且 ImGui 没有在处理这个点击（即没点到按钮）时，才允许拖动整个窗口
        if (glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            if (!isDragging)
            {
                // 修改点：检查是否“没有”点在任何具体的 UI 按钮或组件上
                // 使用 !ImGui::IsAnyItemHovered() 替代 WantCaptureMouse
                if (!ImGui::IsAnyItemActive()) //! ImGui::IsAnyItemHovered() &&
                {
                    double mx, my;
                    glfwGetCursorPos(win, &mx, &my);
                    dragOffsetX = mx;
                    dragOffsetY = my;
                    isDragging = true;
                }
            }
            else
            {
                POINT p;
                GetCursorPos(&p);
                glfwSetWindowPos(win, p.x - (int)dragOffsetX, p.y - (int)dragOffsetY);
            }
        }
        else
        {
            isDragging = false;
        }
       // glfwPollEvents();

        float luma = 0.5f;
        CaptureScreen(hwnd, screenTex, luma);
        LGG::SetScreenTexture(screenTex);
        LGG::SetEnvLuma(luma);

        demo.Update(1.0f / 45.0f);
      //  UpdateWindowRegion(hwnd, demo);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        LGG::BeginFrame();
        demo.Render();

    
        ImGui::Render();
        int fw, fh;
        glfwGetFramebufferSize(win, &fw, &fh);
        LGG::EndFrame((float)fw, (float)fh);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData()); // 文字最后画
        UpdateWindowRegion(hwnd); // ← 在此插入，s_Cmds 已就绪
        
        glfwSwapBuffers(win);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}