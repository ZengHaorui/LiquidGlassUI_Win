// ================================================================
//  LiquidGlassGround.h
//  Liquid Glass UI — ImGui + OpenGL 3.3
// ================================================================
//
//  用法 / Usage:
//    1. 在主循环前: LiquidGlassGround::Init()
//    2. 每帧（ImGui::NewFrame 之后）:
//         LiquidGlassGround::BeginFrame()
//         ... shape.Render() ...
//         ImGui::Render()
//         LiquidGlassGround::EndFrame(screenW, screenH)
//    3. 屏幕捕获纹理（每帧更新）:
//         LiquidGlassGround::SetScreenTexture(tex)
//         LiquidGlassGround::SetEnvLuma(luma)
//
//  依赖 / Dependencies:
//    - ImGui (imgui.h)
//    - OpenGL 3.3+ loader (glad / glew, 需在包含本文件前加载)
//    - C++17
// ================================================================
#pragma once


#include <imgui.h>
#include <functional>
#include <string>
#include <vector>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <cassert>



// ── GL 类型前置 (由调用方加载 glad/glew 等) ───────────────────

namespace LiquidGlassGround
{

    // ================================================================
    //  § 0  内联插值工具
    // ================================================================
    inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
    inline ImVec2 Lerp(ImVec2 a, ImVec2 b, float t) { return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t}; }
    inline ImVec4 Lerp(ImVec4 a, ImVec4 b, float t)
    {
        return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t};
    }

    // ================================================================
    //  § 1  Material Layers
    // ================================================================

    /// 发光层
    struct LightLayer
    {
        ImVec4 emitColor = {1.f, 1.f, 1.f, 1.f}; ///< 发光颜色
        float intensity = 0.f;                   ///< 发光强度 [0, 1]
    };

    /// 纹理层
    struct TextureLayer
    {
        GLuint texID = 0;    ///< GL纹理句柄 (0 = 无纹理)
        float opacity = 0.f; ///< 叠加不透明度 [0, 1]
    };

    /// 颜色层
    struct ColorLayer
    {
        ImVec4 baseColor = {1.f, 1.f, 1.f, 1.f}; ///< 固有颜色
        float tintStrength = 0.f;                ///< 着色强度 [0, 1]
    };

    /// 模糊层
    struct BlurLayer
    {
        float centerBlur = 0.f;  ///< 中心模糊强度 (像素)
        float edgeBlur = 8.f;    ///< 边缘模糊强度 (像素)
        float startMargin = 6.f; ///< 模糊起始边距: 距边缘多少像素时开始过渡
        float endMargin = 0.f;   ///< 模糊结束边距: 距边缘多少像素时达到最大 (0=贴边)
        float sampleCoeff = 1.f; ///< 采样系数 (越大质量越高, 越慢)
    };

    /// 折射层
    struct RefractionLayer
    {
        float refractIndex = 0.12f;   ///< 折射率
        float normalTransition = 6.f; ///< 法线过渡 / 凹凸锐度
        float flatTopRange = 0.35f;   ///< 平顶范围: 0=全弧面, 1=全平
        float dispersionCoeff = 0.f;  ///< 色散系数 (彩虹边缘)
    };

    /// 阴影层
    struct ShadowLayer
    {
       // bool enabled=0;
        ImVec2 direction = {3.f, 5.f}; ///< 阴影偏移方向 (像素)
        float size = 10.f;             ///< 阴影扩散大小
        float opacity = 0.35f;         ///< 阴影透明度 [0, 1]
        float blur = 12.f;             ///< 阴影模糊半径
    };

    // ================================================================
    //  § 2  glass_material
    // ================================================================
    struct glass_material
    {
        LightLayer light;
        TextureLayer texture;
        ColorLayer color;
        BlurLayer blur;
        RefractionLayer refraction;
        ShadowLayer shadow;
        float alpha = 1.f; ///< 整体透明度

        /// 在 a 与 b 之间按 k ∈ [0,1] 线性插值 (0→a, 1→b)
        static glass_material transform(const glass_material &a, const glass_material &b, float k)
        {
            glass_material r;
            r.light.emitColor = Lerp(a.light.emitColor, b.light.emitColor, k);
            r.light.intensity = Lerp(a.light.intensity, b.light.intensity, k);
            r.color.baseColor = Lerp(a.color.baseColor, b.color.baseColor, k);
            r.color.tintStrength = Lerp(a.color.tintStrength, b.color.tintStrength, k);
            r.blur.centerBlur = Lerp(a.blur.centerBlur, b.blur.centerBlur, k);
            r.blur.edgeBlur = Lerp(a.blur.edgeBlur, b.blur.edgeBlur, k);
            r.blur.startMargin = Lerp(a.blur.startMargin, b.blur.startMargin, k);
            r.blur.endMargin = Lerp(a.blur.endMargin, b.blur.endMargin, k);
            r.blur.sampleCoeff = Lerp(a.blur.sampleCoeff, b.blur.sampleCoeff, k);
            r.refraction.refractIndex = Lerp(a.refraction.refractIndex, b.refraction.refractIndex, k);
            r.refraction.normalTransition = Lerp(a.refraction.normalTransition, b.refraction.normalTransition, k);
            r.refraction.flatTopRange = Lerp(a.refraction.flatTopRange, b.refraction.flatTopRange, k);
            r.refraction.dispersionCoeff = Lerp(a.refraction.dispersionCoeff, b.refraction.dispersionCoeff, k);
            r.shadow.direction = Lerp(a.shadow.direction, b.shadow.direction, k);
            r.shadow.size = Lerp(a.shadow.size, b.shadow.size, k);
            r.shadow.opacity = Lerp(a.shadow.opacity, b.shadow.opacity, k);
            r.shadow.blur = Lerp(a.shadow.blur, b.shadow.blur, k);
            r.alpha = Lerp(a.alpha, b.alpha, k);
            return r;
        }
    };

    // ================================================================
    //  § 3  glass_pos
    // ================================================================
    struct glass_pos
    {
        float width = 100.f;      ///< 宽
        float height = 100.f;     ///< 高
        float x = 0.f;            ///< 相对上级位置 X
        float y = 0.f;            ///< 相对上级位置 Y
        float cornerRadius = 8.f; ///< 圆角弧度

        ImVec2 GetPos() const { return {x, y}; }
        ImVec2 GetSize() const { return {width, height}; }

        /// 在 a 与 b 之间按 k ∈ [0,1] 线性插值
        static glass_pos transform(const glass_pos &a, const glass_pos &b, float k)
        {
            return {Lerp(a.width, b.width, k), Lerp(a.height, b.height, k),
                    Lerp(a.x, b.x, k), Lerp(a.y, b.y, k),
                    Lerp(a.cornerRadius, b.cornerRadius, k)};
        }
    };

    // ================================================================
    //  § 4  内部渲染命令
    // ================================================================
    enum class ShapeKind
    {
        RoundRect,
        Circle
    };

    struct GlassCmd
    {
        ShapeKind kind = ShapeKind::RoundRect;
        glass_pos pos;
        glass_material mat;
        GLuint normalTex = 0; ///< 法线贴图 (文字/图标专用, 0=使用SDF法线)
    };

    /// 每帧全局命令列表 (BeginFrame() 清空)
    static std::vector<GlassCmd> s_Cmds;

    // ================================================================
    //  § 5  Shader Sources
    // ================================================================
    namespace Detail
    {

        // ──────────────────────────── 公共顶点着色器 ────────────────────
        static const char *kCommonVS = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
uniform vec2 uPos, uSize, uScreenSize;
out vec2 vFragPos;
out vec2 vUV;
void main() {
    vec2 px  = uPos + aPos * uSize;
    vFragPos = px;
    vUV      = aPos;
    vec2 ndc = px / uScreenSize * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
}
)GLSL";

// ──────────────────────────── 阴影着色器 ────────────────────────
static const char *kShadowVS = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
uniform vec2  uPos, uSize, uScreenSize, uShadowDir;
uniform float uExpand;
out vec2 vFragPos;
void main() {
    vec2 qMin = uPos + uShadowDir - vec2(uExpand);
    vec2 qMax = uPos + uShadowDir + uSize + vec2(uExpand);
    vec2 px   = qMin + aPos * (qMax - qMin);
    vFragPos  = px;
    vec2 ndc  = px / uScreenSize * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
}
)GLSL";

static const char *kShadowFS = R"GLSL(
#version 330 core
in  vec2 vFragPos;
out vec4 FragColor;
uniform vec2  uPos, uSize, uShadowDir;
uniform float uCornerRadius, uOpacity, uBlur;

float sdf(vec2 p, vec2 b, float r) {
    vec2 d = abs(p) - b + r;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - r;
}
void main() {
    vec2  c = uPos + uShadowDir + uSize * 0.5;
    float d = sdf(vFragPos - c, uSize * 0.5, uCornerRadius);
    // Gaussian-like soft shadow
    float a = exp(-max(d, 0.0) / max(uBlur, 0.001));
    a *= smoothstep(uBlur * 2.0, 0.0, d) * uOpacity;
    FragColor = vec4(0.0, 0.0, 0.0, clamp(a, 0.0, 1.0));
}
)GLSL";

// ──────────────────────────── B版：圆形阴影 ────────────────────────
static const char *kShadowVS_B = R"GLSL(
#version 330 core
layout(location=0) in vec2 aPos;
uniform vec2  uPos, uScreenSize, uShadowDir;
uniform float uRadius, uExpand; // 圆形只需要半径
out vec2 vFragPos;
void main() {
    // 计算圆心的世界坐标
    vec2 center = uPos + uShadowDir + vec2(uRadius);
    // 绘制区域是半径 + 扩展（模糊）范围
    float totalR = uRadius + uExpand;
    vec2 qMin = center - vec2(totalR);
    vec2 qMax = center + vec2(totalR);
    
    vec2 px   = qMin + aPos * (qMax - qMin);
    vFragPos  = px;
    vec2 ndc  = px / uScreenSize * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
}
)GLSL";

static const char *kShadowFS_B = R"GLSL(
#version 330 core
in  vec2 vFragPos;
out vec4 FragColor;
uniform vec2  uPos, uShadowDir;
uniform float uRadius, uOpacity, uBlur;

void main() {
    vec2  center = uPos + uShadowDir + vec2(uRadius);
    // 圆形 SDF: 点到圆心的距离 - 半径
    float d = length(vFragPos - center) - uRadius;
    
    // 软阴影计算
    // 当 d < 0 时在圆内，d > 0 时在圆外开始扩散
    float a = exp(-max(d, 0.0) / max(uBlur, 0.001));
    
    // 边缘平滑处理
    a *= smoothstep(uBlur * 2.5, 0.0, d) * uOpacity;
    
    FragColor = vec4(0.0, 0.0, 0.0, clamp(a, 0.0, 1.0));
}
)GLSL";

// ──────────────────────────── 圆角矩形玻璃着色器 ────────────────
static const char *kGlassFS_RoundRect = R"GLSL(
#version 330 core
in  vec2 vFragPos;
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScreenTex;
uniform sampler2D uUserTex;
uniform sampler2D uNormalTex;
uniform bool  uHasUserTex;
uniform bool  uHasNormalTex;

uniform vec2  uScreenSize, uPos, uSize;
uniform float uCornerRadius, uAlpha, uLuma;

// 发光层
uniform vec4  uEmitColor;
uniform float uEmitIntensity;
// 颜色层
uniform vec4  uBaseColor;
uniform float uTintStrength;
// 模糊层
uniform float uCenterBlur, uEdgeBlur, uBlurStart, uBlurEnd;
// 折射层
uniform float uRefractIndex, uNormalTransition, uFlatTopRange, uDispersion;

// ── SDF ────────────────────────────────────────────────────────
float sdf(vec2 p, vec2 b, float r) {
    vec2 d = abs(p) - b + r;
    return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - r;
}

// 圆角矩形 SDF 解析梯度 (= 外法线 XY)
vec2 sdfNormal(vec2 p, vec2 b, float r) {
    vec2 ap = abs(p);
    vec2 d  = ap - b + r;
    vec2 s  = sign(p);
    if (d.x > 0.0 && d.y > 0.0) return s * normalize(d);
    return (d.x > d.y) ? vec2(s.x, 0.0) : vec2(0.0, s.y);
}

// ── 旋转网格8采样模糊 ──────────────────────────────────────────
vec3 blur8(vec2 uv, float radius) {
    if (radius < 0.5) return texture(uScreenTex, uv).rgb;
    float r = radius / min(uScreenSize.x, uScreenSize.y);
    const vec2 off[8] = vec2[8](
        vec2( 1.000,  0.000), vec2(-1.000,  0.000),
        vec2( 0.000,  1.000), vec2( 0.000, -1.000),
        vec2( 0.707,  0.707), vec2(-0.707,  0.707),
        vec2( 0.707, -0.707), vec2(-0.707, -0.707));
    vec3 col = texture(uScreenTex, uv).rgb;
    for (int i = 0; i < 8; i++)
        col += texture(uScreenTex, uv + off[i] * r).rgb;
    return col / 9.0;
}

void main() {
    vec2 center   = uPos + uSize * 0.5;
    vec2 halfSize = uSize * 0.5;
    vec2 p        = vFragPos - center;

    float dist = sdf(p, halfSize, uCornerRadius);
    if (dist > 0.5) discard;
    
    // 1. 基础遮罩（矩形区域）
    float mask = smoothstep(0.5, -0.5, dist);
    
    // --- 【关键修改点 A】 ---
    // 如果有法线贴图（即渲染文字），用文字的 Alpha 覆盖矩形遮罩
    float textAlpha = 1.0;
    if (uHasNormalTex) {
        textAlpha = texture(uNormalTex, vUV).a;
        mask *= textAlpha; // 只有文字部分不透明，矩形其余部分变为透明
    }
    
    if (mask < 0.001) discard; // 如果完全透明则不渲染，提高性能并彻底消除底板
    // ------------------------

    float edgeDist = max(-dist, 0.0);

    // ── 法线计算 ──────────────────────────────────────────────
    vec2 nxy;
    if (uHasNormalTex) {
        vec3 nt  = texture(uNormalTex, vUV).rgb;
        vec2 raw = nt.xy * 2.0 - 1.0;
        nxy = raw * nt.z; 
    } else {
        vec2 rawN   = sdfNormal(p, halfSize, uCornerRadius);
        float minH  = min(halfSize.x, halfSize.y);
        float flatarea  = minH * uFlatTopRange;
        float cm    = 1.0 - smoothstep(0.0, flatarea, edgeDist);
        float power = max(uNormalTransition * 0.4 + 1.0, 1.0);
        nxy = rawN * pow(cm, power);
    }
    float nz = sqrt(max(0.0, 1.0 - dot(nxy, nxy)));

    // ── 折射 UV ──────────────────────────────────────────────
    vec2 screenUV = vFragPos / uScreenSize;
    vec2 refr     = nxy * uRefractIndex;

    // ── 模糊强度 (按位置混合中心/边缘模糊) ───────────────────
    float blurT = 1.0 - smoothstep(uBlurEnd, uBlurStart, edgeDist);
    float blurR = mix(uCenterBlur, uEdgeBlur, blurT);

    // ── 采样背景 (支持色散) ──────────────────────────────────
    vec3 col;
    if (uDispersion > 0.001) {
        float rd = blur8(screenUV + refr * (1.0 + uDispersion), blurR).r;
        float gd = blur8(screenUV + refr,                        blurR).g;
        float bd = blur8(screenUV + refr * (1.0 - uDispersion), blurR).b;
        col = vec3(rd, gd, bd);
    } else {
        col = blur8(screenUV + refr, blurR);
    }

    // ── 固有颜色着色 ─────────────────────────────────────────
    col = mix(col, uBaseColor.rgb, uTintStrength * uBaseColor.a);

    // ── 用户纹理叠加 (图标等) ─────────────────────────────────
    if (uHasUserTex) {
        vec4 t = texture(uUserTex, vUV);
        col    = mix(col, t.rgb, t.a);
    }

    // ── Fresnel 高光 ──────────────────────────────────────────
    // uLuma: 0=暗环境(白色高光), 1=亮环境(灰色高光)
    vec3 lightCol = mix(vec3(1.0), vec3(0.1), uLuma * 0.85);
    float fresnel = pow(clamp(1.0 - nz, 0.0, 1.0), 3.0);
    col = mix(col, lightCol, 0.04 + fresnel * 0.32);

// ── 边缘高光环 ────────────────────────────────────────────
    float rim = pow(smoothstep(3.0, 0.0, abs(dist)), 3.0);
    
    // --- 【修改】如果存在字形法线贴图(文本)，不绘制矩形的外边框高光 ---
    if (uHasNormalTex) {
        rim = 0.0;
    }
    // -------------------------------------------------------------
    
    col += lightCol * rim * 0.5;

    // ── 发光 ──────────────────────────────────────────────────
    if (uEmitIntensity > 0.001)
        col = mix(col, uEmitColor.rgb, uEmitIntensity * uEmitColor.a);

    FragColor = vec4(col, mask * uAlpha);
}
)GLSL";

// ──────────────────────────── 圆形玻璃着色器 ────────────────────
static const char *kGlassFS_Circle = R"GLSL(
#version 330 core
in  vec2 vFragPos;
in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uScreenTex;
uniform sampler2D uUserTex;
uniform bool  uHasUserTex;
uniform vec2  uScreenSize, uPos, uSize;
uniform float uAlpha, uLuma;
uniform vec4  uEmitColor;
uniform float uEmitIntensity;
uniform vec4  uBaseColor;
uniform float uTintStrength;
uniform float uCenterBlur, uEdgeBlur, uBlurStart, uBlurEnd;
uniform float uRefractIndex, uFlatTopRange, uDispersion;

vec3 blur8(vec2 uv, float r) {
    if (r < 0.5) return texture(uScreenTex, uv).rgb;
    float s = r / min(uScreenSize.x, uScreenSize.y);
    const vec2 off[8] = vec2[8](
        vec2(1,0),vec2(-1,0),vec2(0,1),vec2(0,-1),
        vec2(0.707,0.707),vec2(-0.707,0.707),
        vec2(0.707,-0.707),vec2(-0.707,-0.707));
    vec3 col = texture(uScreenTex, uv).rgb;
    for (int i = 0; i < 8; i++) col += texture(uScreenTex, uv + off[i]*s).rgb;
    return col / 9.0;
}

void main() {
    vec2  center = uPos + uSize * 0.5;
    float rad    = min(uSize.x, uSize.y) * 0.5;
    vec2  p      = vFragPos - center;

    float dist = length(p) - rad;
    if (dist > 0.5) discard;
    float mask     = smoothstep(0.5, -0.5, dist);
    float edgeDist = max(-dist, 0.0);

    // 球面法线
    vec2  pn   = p / (rad + 0.0001);
    float flatarea = rad * uFlatTopRange;
    float cm   = 1.0 - smoothstep(0.0, flatarea, edgeDist);
    vec2  nxy  = pn * cm;
    float nz   = sqrt(max(0.0, 1.0 - dot(nxy, nxy)));

    vec2 screenUV = vFragPos / uScreenSize;
    vec2 refr     = nxy * uRefractIndex;
    float blurT   = 1.0 - smoothstep(uBlurEnd, uBlurStart, edgeDist);
    float blurR   = mix(uCenterBlur, uEdgeBlur, blurT);

    vec3 col;
    if (uDispersion > 0.001) {
        float rd = blur8(screenUV + refr*(1.0+uDispersion), blurR).r;
        float gd = blur8(screenUV + refr,                   blurR).g;
        float bd = blur8(screenUV + refr*(1.0-uDispersion), blurR).b;
        col = vec3(rd, gd, bd);
    } else {
        col = blur8(screenUV + refr, blurR);
    }

    col = mix(col, uBaseColor.rgb, uTintStrength * uBaseColor.a);
    if (uHasUserTex) { vec4 t = texture(uUserTex, vUV); col = mix(col, t.rgb, t.a); }

    vec3  lightCol = mix(vec3(1.0), vec3(0.1), uLuma * 0.85);
    float fresnel  = pow(clamp(1.0 - nz, 0.0, 1.0), 3.0);
    col = mix(col, lightCol, 0.04 + fresnel * 0.32);
    float rim = pow(smoothstep(3.0, 0.0, abs(dist)), 3.0);
    col += lightCol * rim * 0.5;

    if (uEmitIntensity > 0.001)
        col = mix(col, uEmitColor.rgb, uEmitIntensity * uEmitColor.a);

    FragColor = vec4(col, mask * uAlpha);
}
)GLSL";

    } // namespace Detail

    // ================================================================
    //  § 6  法线贴图工具 (CPU Sobel, 无需额外FBO)
    // ================================================================
    namespace NormalMap
    {

        /// 从 RGBA 像素数组的 alpha 通道生成法线贴图纹理
        /// 编码格式: R=nx*0.5+0.5, G=ny*0.5+0.5, B=strength(=alpha), A=alpha
        inline GLuint FromAlphaBuffer(const uint8_t *rgba, int w, int h)
        {
            std::vector<uint8_t> out(w * h * 4, 128);

            auto A = [&](int x, int y) -> float
            {
                x = std::clamp(x, 0, w - 1);
                y = std::clamp(y, 0, h - 1);
                return rgba[(y * w + x) * 4 + 3] / 255.f;
            };

            for (int y = 0; y < h; y++)
            {
                for (int x = 0; x < w; x++)
                {
                    // Sobel
                    float gx = (-A(x - 1, y - 1) - 2 * A(x - 1, y) - A(x - 1, y + 1) + A(x + 1, y - 1) + 2 * A(x + 1, y) + A(x + 1, y + 1)) * 0.25f;
                    float gy = (-A(x - 1, y - 1) - 2 * A(x, y - 1) - A(x + 1, y - 1) + A(x - 1, y + 1) + 2 * A(x, y + 1) + A(x + 1, y + 1)) * 0.25f;
                    float a = A(x, y);
                    int idx = (y * w + x) * 4;
                    out[idx + 0] = (uint8_t)((gx * 0.5f + 0.5f) * 255);
                    out[idx + 1] = (uint8_t)((gy * 0.5f + 0.5f) * 255);
                    out[idx + 2] = (uint8_t)(a * 255); // 折射强度
                    out[idx + 3] = (uint8_t)(a * 255);
                }
            }

            GLuint tex;
            glGenTextures(1, &tex);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, out.data());
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
            return tex;
        }

        /// 从 ImGui 字体图集生成指定文本的法线贴图
        /// 需在 ImGui 字体初始化完成后调用
        /// @param font  使用的字体 (nullptr = 默认字体)
        // ================================================================
            //  § 6  法线贴图工具 - 修正版
            // ================================================================
        /// 从 ImGui 字体图集生成指定文本的法线贴图 (适配 ImGui 1.92.0+)
        inline GLuint FromImGuiText(const char *text, ImFont *font = nullptr)
        {
            // 修复点 1: 使用 GetFont() 获取当前字体
            ImFont *f = font ? font : ImGui::GetFont();
            if (!f || !text || !*text)
                return 0;

            // 修复点 2: 获取当前渲染尺寸
            // 1.92+ 建议使用 ImGui::GetFontSize()
            float currentFontSize = ImGui::GetFontSize();

            // 修复点 3: 获取对应的烘焙数据 (Baked Data)
            // 所有的字形查找 (FindGlyph) 现在都在 Baked Data 中
            ImFontBaked *baked = f->GetFontBaked(currentFontSize);
            if (!baked)
                return 0;

            // 获取字体图集像素 (RGBA)
            unsigned char *atlasPx = nullptr;
            int atlasW = 0, atlasH = 0;
            ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&atlasPx, &atlasW, &atlasH);
            if (!atlasPx)
                return 0;

            // 计算文本渲染尺寸 (使用新的 CalcTextSizeA 签名)
            ImVec2 ts = f->CalcTextSizeA(currentFontSize, FLT_MAX, 0.f, text);
            int tw = std::max(1, (int)std::ceil(ts.x));
            int th = std::max(1, (int)std::ceil(ts.y));

            // 光栅化文字 alpha 到 CPU 缓冲区
            std::vector<uint8_t> buf(tw * th * 4, 0);

            float penX = 0.f;
            const char *s = text;
            while (*s)
            {
                unsigned int c = (unsigned char)*s++;

                // 修复点 4: 从 baked 对象查找字形
                const ImFontGlyph *g = baked->FindGlyph((ImWchar)c);

                if (!g || !g->Visible)
                {
                    penX += g ? g->AdvanceX : (currentFontSize * 0.5f);
                    continue;
                }

                int srcX0 = (int)(g->U0 * atlasW);
                int srcY0 = (int)(g->V0 * atlasH);
                int gW = (int)std::ceil((g->U1 - g->U0) * atlasW);
                int gH = (int)std::ceil((g->V1 - g->V0) * atlasH);

                // 修正渲染坐标 (g->Y0 是相对于基线的偏移)
                int dstX = (int)(penX + g->X0);
                int dstY = (int)g->Y0;

                for (int py = 0; py < gH; py++)
                {
                    int dy = dstY + py;
                    if (dy < 0 || dy >= th)
                        continue;
                    for (int px = 0; px < gW; px++)
                    {
                        int dx = dstX + px;
                        if (dx < 0 || dx >= tw)
                            continue;

                        int srcIdx = ((srcY0 + py) * atlasW + (srcX0 + px)) * 4;
                        uint8_t a = atlasPx[srcIdx + 3];
                        int dstIdx = (dy * tw + dx) * 4;

                        if (a > buf[dstIdx + 3])
                        {
                            buf[dstIdx + 0] = 255;
                            buf[dstIdx + 1] = 255;
                            buf[dstIdx + 2] = 255;
                            buf[dstIdx + 3] = a;
                        }
                    }
                }
                penX += g->AdvanceX;
            }

            // CPU Sobel → 法线贴图
            return FromAlphaBuffer(buf.data(), tw, th);
        }

        /// 从已有 GL 纹理的 alpha 通道生成法线贴图 (需要下载像素, 较慢)
        inline GLuint FromGLTexture(GLuint srcTex, int w, int h)
        {
            std::vector<uint8_t> px(w * h * 4);
            glBindTexture(GL_TEXTURE_2D, srcTex);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
            glBindTexture(GL_TEXTURE_2D, 0);
            return FromAlphaBuffer(px.data(), w, h);
        }

    } // namespace NormalMap

    // ================================================================
    //  § 7  Renderer (单例)
    // ================================================================
    class Renderer
    {
    public:
        GLuint vao = 0, vbo = 0;
        GLuint progShadow = 0, progRR = 0, progCircle = 0;
        GLuint screenTex = 0;
        float envLuma = 0.5f;
        bool ready = false;

        static Renderer &Get()
        {
            static Renderer r;
            return r;
        }

        static GLuint CompileShader(GLenum type, const char *src)
        {
            GLuint h = glCreateShader(type);
            glShaderSource(h, 1, &src, nullptr);
            glCompileShader(h);

            GLint ok;
            glGetShaderiv(h, GL_COMPILE_STATUS, &ok);
            if (!ok)
            {
                char buf[2048];
                glGetShaderInfoLog(h, 2048, nullptr, buf);
                // 如果你没有控制台，可以使用 OutputDebugStringA (Windows.h)
                printf("Shader Error: %s\n", buf);
            }
            return h;
        }

        static GLuint LinkProg(const char *vs, const char *fs)
        {
            GLuint p = glCreateProgram();
            glAttachShader(p, CompileShader(GL_VERTEX_SHADER, vs));
            glAttachShader(p, CompileShader(GL_FRAGMENT_SHADER, fs));
            glLinkProgram(p);
            return p;
        }

        void Init()
        {
            if (ready)
                return;
            float quad[] = {0, 0, 1, 0, 0, 1, 1, 1};
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 8, nullptr);
            glBindVertexArray(0);

            //progShadow = LinkProg(Detail::kShadowVS, Detail::kShadowFS);
            progRR = LinkProg(Detail::kCommonVS, Detail::kGlassFS_RoundRect);
            progCircle = LinkProg(Detail::kCommonVS, Detail::kGlassFS_Circle);
            ready = true;
        }

        // 设置材质 uniform (通用)
        void SetMatUniforms(GLuint prog, const glass_material &m)
        {
            auto u = [&](const char *n)
            { return glGetUniformLocation(prog, n); };
            glUniform1f(u("uAlpha"), m.alpha);
            glUniform1f(u("uLuma"), envLuma);
            glUniform4f(u("uEmitColor"), m.light.emitColor.x, m.light.emitColor.y,
                        m.light.emitColor.z, m.light.emitColor.w);
            glUniform1f(u("uEmitIntensity"), m.light.intensity);
            glUniform4f(u("uBaseColor"), m.color.baseColor.x, m.color.baseColor.y,
                        m.color.baseColor.z, m.color.baseColor.w);
            glUniform1f(u("uTintStrength"), m.color.tintStrength);
            glUniform1f(u("uCenterBlur"), m.blur.centerBlur);
            glUniform1f(u("uEdgeBlur"), m.blur.edgeBlur);
            glUniform1f(u("uBlurStart"), m.blur.startMargin);
            glUniform1f(u("uBlurEnd"), m.blur.endMargin);
            glUniform1f(u("uRefractIndex"), m.refraction.refractIndex);
            glUniform1f(u("uNormalTransition"), m.refraction.normalTransition);
            glUniform1f(u("uFlatTopRange"), m.refraction.flatTopRange);
            glUniform1f(u("uDispersion"), m.refraction.dispersionCoeff);
        }

        void DrawCmd(const GlassCmd &cmd, float sw, float sh)
        {
            const glass_pos &pos = cmd.pos;
            const glass_material &mat = cmd.mat;

            // ── 阴影 pass ──────────────────────────────────────
            if (mat.shadow.opacity > 0.01f && mat.alpha > 0.01f)
            {
                glUseProgram(progShadow);
                glBindVertexArray(vao);
                auto u = [&](const char *n)
                { return glGetUniformLocation(progShadow, n); };
                glUniform2f(u("uPos"), pos.x, pos.y);
                glUniform2f(u("uSize"), pos.width, pos.height);
                glUniform2f(u("uScreenSize"), sw, sh);
                glUniform2f(u("uShadowDir"), mat.shadow.direction.x, mat.shadow.direction.y);
                glUniform1f(u("uExpand"), mat.shadow.size);
                glUniform1f(u("uCornerRadius"), pos.cornerRadius);
                glUniform1f(u("uOpacity"), mat.shadow.opacity * mat.alpha);
                glUniform1f(u("uBlur"), mat.shadow.blur);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            }

            // ── 玻璃 pass ──────────────────────────────────────
            GLuint prog = (cmd.kind == ShapeKind::Circle) ? progCircle : progRR;
            glUseProgram(prog);
            glBindVertexArray(vao);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, screenTex);
            glUniform1i(glGetUniformLocation(prog, "uScreenTex"), 0);

            // 用户纹理 (图标/自定义)
            bool hasUserTex = (mat.texture.texID != 0 && mat.texture.opacity > 0.f);
            if (hasUserTex)
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, mat.texture.texID);
                glUniform1i(glGetUniformLocation(prog, "uUserTex"), 1);
            }
            glUniform1i(glGetUniformLocation(prog, "uHasUserTex"), (GLint)hasUserTex);

            // 法线贴图 (文字/图标, 仅 RoundRect shader 支持)
            bool hasNormalTex = (cmd.normalTex != 0 && prog == progRR);
            if (hasNormalTex)
            {
                glActiveTexture(GL_TEXTURE2);
                glBindTexture(GL_TEXTURE_2D, cmd.normalTex);
                glUniform1i(glGetUniformLocation(prog, "uNormalTex"), 2);
            }
            if (prog == progRR)
                glUniform1i(glGetUniformLocation(prog, "uHasNormalTex"), (GLint)hasNormalTex);

            auto u = [&](const char *n)
            { return glGetUniformLocation(prog, n); };
            glUniform2f(u("uPos"), pos.x, pos.y);
            glUniform2f(u("uSize"), pos.width, pos.height);
            glUniform2f(u("uScreenSize"), sw, sh);
            glUniform1f(u("uCornerRadius"), pos.cornerRadius);
            SetMatUniforms(prog, mat);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }

        // 在 LiquidGlassGround.h 的 Renderer 类中修改
        void RenderAll(float sw, float sh)
        {
            if (s_Cmds.empty())
                return;

            // --- 强制状态重置 ---
            glDisable(GL_SCISSOR_TEST); // 极其重要：ImGui 经常开启这个
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glViewport(0, 0, (GLsizei)sw, (GLsizei)sh);

            for (const auto &cmd : s_Cmds)
                DrawCmd(cmd, sw, sh);

            // 结束后清理绑定的程序和 VAO
            glUseProgram(0);
            glBindVertexArray(0);
        }
    };

    // ================================================================
    //  § 8  base — 事件响应基类
    // ================================================================
    class base
    {
    public:
        std::function<void()> on_hover;      ///< 悬停时
        std::function<void()> on_unhover;    ///< 取消悬停时

        std::function<void()> on_click;      ///< 点击时
        std::function<void()> on_press;      ///< 按压时
        std::function<void()> on_release;    ///< 释放时
        std::function<void()> on_drag;       ///< 拖动时
        std::function<void()> on_focus;      ///< 获得焦点时
        std::function<void()> on_lose_focus; ///< 失去焦点时
/*
        std::function<void()> on_create;     ///< 创建时
        std::function<void()> on_destroy;    ///< 销毁时

        std::function<void()> on_enable;     ///< 启用时
        std::function<void()> on_disable;    ///< 禁用时
*/
    protected:
        bool m_prevHover = false;
        bool m_prevFocus = false;

        /// 通过 ImGui InvisibleButton 处理命中检测与事件
        void ProcessEvents(const char *id, const glass_pos &pos)
        {
            ImGui::SetCursorScreenPos({pos.x, pos.y});
            ImGui::InvisibleButton(id, {pos.width, pos.height});

            bool hov = ImGui::IsItemHovered();
            bool act = ImGui::IsItemActive();
            bool clk = ImGui::IsItemClicked();
            bool foc = ImGui::IsItemFocused();
            bool rel = !act && m_prevHover && ImGui::IsMouseReleased(0);

            if (hov && !m_prevHover && on_hover) on_hover();
            if (!hov && m_prevHover && on_unhover) on_unhover();

            if (clk && on_click) on_click();
            if (act && on_press) on_press();
            if (rel && on_release) on_release();
            if (act && ImGui::IsMouseDragging(0) && on_drag) on_drag();
            if (foc && !m_prevFocus && on_focus) on_focus();
            if (!foc && m_prevFocus && on_lose_focus) on_lose_focus();
            
            m_prevHover = hov;
            m_prevFocus = foc;
        }
    };

    // ================================================================
    //  § 9  Shape — 圆角矩形
    // ================================================================
    class RoundRect : public base
    {
    public:
        glass_pos pos,target_pos;
        glass_material mat, target_mat;
        std::string id;
        int animframe_pos=0,animframe_mat=0;

        RoundRect() : id("rr") {}
        explicit RoundRect(const char *id) : id(id) {}
        RoundRect(glass_pos p, glass_material m, const char *id = "rr")
            : pos(p), mat(m), id(id), target_pos(p), target_mat(m) {}


        void setTargetPosition(glass_pos p,int frame)
        {
            target_pos = p;
            animframe_pos = frame;
        }

        void setTargetMaterial(glass_material m,int frame)
        {
            target_mat = m;
            animframe_mat = frame;
        }
        void Render()
        {
            ProcessEvents(id.c_str(), pos);

            if (animframe_pos > 0)
            {
                pos = pos.transform(pos, target_pos, 1.0f / animframe_pos);
                animframe_pos--;
            }
            if (animframe_mat > 0)
            {
                mat = mat.transform(mat, target_mat, 1.0f / animframe_mat);
                animframe_mat--;
            }
            s_Cmds.push_back({ShapeKind::RoundRect, pos, mat, 0});
        }
    };

    // ================================================================
    //  § 10  Shape — 圆形
    // ================================================================
    class Circle : public base
    {
    public:
        glass_pos pos,target_pos; ///< width/height 均为直径; cornerRadius 无效
        glass_material mat,target_mat;
        std::string id;
        int animframe_pos=0,animframe_mat=0;
        Circle() : id("circle") {}
        explicit Circle(const char *id) : id(id) {}
        Circle(glass_pos p, glass_material m, const char *id = "circle")
            : pos(p), mat(m), id(id), target_pos(p), target_mat(m)
        {
            // 强制为正方形 (取较小边)
            float d = std::min(p.width, p.height);
            this->pos.width = this->pos.height = d;
        }

        void setTargetPosition(glass_pos p, int frame)
        {
            target_pos = p;
            animframe_pos = frame;
        }

        void setTargetMaterial(glass_material m, int frame)
        {
            target_mat = m;
            animframe_mat = frame;
        }

        void Render()
        {
            ProcessEvents(id.c_str(), pos);
            
            if(animframe_pos>0){
                pos=pos.transform(pos,target_pos,1.0f/animframe_pos);
                animframe_pos--;
            }
            if(animframe_mat>0){
                mat=mat.transform(mat,target_mat,1.0f/animframe_mat);
                animframe_mat--;
            }

            s_Cmds.push_back({ShapeKind::Circle, pos, mat, 0});
        }
    };

    // ================================================================
    //  § 11  Shape — 文本
    //
    //  渲染逻辑:
    //    1. 玻璃背景 (RoundRect SDF 法线)
    //    2. 若已调用 BakeNormalMap(), 则用字形法线贴图驱动折射
    //       使文字形状在玻璃面上产生物理凸起效果
    //    3. 在 ImGui DrawList 上叠加文字
    // ================================================================
    class Text : public base
    {
    public:
        glass_pos pos;
        glass_material mat;
        std::string text;
        ImFont *font = nullptr; ///< 字体 (nullptr = ImGui 默认字体)
        float fontSize = 16.f;
        bool autoSize = true; ///< 自动按文本调整 pos.width/height

        GLuint normalTex = 0; ///< 字形法线贴图 (调用 BakeNormalMap() 生成)

        Text() = default;
        Text(const char *txt, float fs = 16.f, ImFont *f = nullptr)
            : text(txt), fontSize(fs), font(f) {}
        Text(glass_pos p, glass_material m, const char *txt, float fs = 16.f, ImFont *f = nullptr)
            : pos(p), mat(m), text(txt), fontSize(fs), font(f) {}

        /// 从字体图集烘焙字形法线贴图
        /// 调用后, 文字区域的折射将跟随字形轮廓 (产生浮雕/凹凸感)
        /// 需在 ImGui 字体初始化完成后调用一次
        void BakeNormalMap()
        {
            if (normalTex)
            {
                glDeleteTextures(1, &normalTex);
                normalTex = 0;
            }
            normalTex = NormalMap::FromImGuiText(text.c_str(), font);
        }

        void Render()
        {
            if (autoSize)
            {
                ImGui::PushFont(font);
                ImVec2 ts = ImGui::CalcTextSize(text.c_str());
                ImGui::PopFont();
                pos.width = std::max(pos.width, ts.x + 16.f);
                pos.height = std::max(pos.height, ts.y + 8.f);
            }


            mat.shadow.direction = ImVec2(0, 0);
            mat.shadow.size = 0;
            mat.shadow.opacity = 0;

            std::string bid = "txt_" + text;
            ProcessEvents(bid.c_str(), pos);

            // 推入玻璃渲染命令
            s_Cmds.push_back({ShapeKind::RoundRect, pos, mat, normalTex});

            // ── ImGui DrawList 文字叠加 ──────────────────────
            ImDrawList *dl = ImGui::GetWindowDrawList();
            ImGui::PushFont(font);
            ImVec2 ts = ImGui::CalcTextSize(text.c_str());
            ImVec2 tp = {pos.x + (pos.width - ts.x) * 0.5f,
                         pos.y + (pos.height - ts.y) * 0.5f};

            // 根据环境亮度自适应文字颜色
            //float luma = Renderer::Get().envLuma;
            //ImU32 fg = (luma > 0.5f) ? IM_COL32(20, 20, 20, 210) : IM_COL32(240, 240, 240, 210);
            //ImU32 sh = IM_COL32(0, 0, 0, 60);
            // 文字软阴影
            //dl->AddText(nullptr, fontSize, {tp.x + 1.f, tp.y + 1.f}, sh, text.c_str());
            //dl->AddText(nullptr, fontSize, tp, fg, text.c_str());
            ImGui::PopFont();
        }
    };

    // ================================================================
    //  § 12  Shape — 图标 (SVG 预光栅化纹理)
    //
    //  使用方式:
    //    1. 在外部将 SVG 光栅化为 RGBA 纹理 (nanosvg + stb_image_write,
    //       或 librsvg, 或任意方式)
    //    2. 调用 BakeNormalMap(w, h) 从 alpha 通道生成法线贴图
    //    3. 每帧调用 Render()
    //
    //  法线计算:
    //    Sobel 算子作用于图标 alpha 通道, 将图标轮廓转为切线空间法线,
    //    使图标在玻璃面上產生物理凸起/折射效果
    // ================================================================
    class Icon : public base
    {
    public:
        glass_pos pos;
        glass_material mat;
        GLuint iconTex = 0;   ///< 图标 RGBA 纹理
        GLuint normalTex = 0; ///< 法线贴图 (由 BakeNormalMap 生成)
        std::string id;

        Icon() : id("icon") {}
        explicit Icon(const char *id) : id(id) {}
        Icon(glass_pos p, glass_material m, GLuint tex, const char *id = "icon")
            : pos(p), mat(m), iconTex(tex), id(id) {}

        /// 从图标纹理的 alpha 通道烘焙法线贴图
        /// @param w, h  纹理宽高 (像素)
        void BakeNormalMap(int w, int h)
        {
            if (!iconTex)
                return;
            if (normalTex)
            {
                glDeleteTextures(1, &normalTex);
                normalTex = 0;
            }
            normalTex = NormalMap::FromGLTexture(iconTex, w, h);
        }

        void Render()
        {
            ProcessEvents(id.c_str(), pos);

            GlassCmd cmd;
            cmd.kind = ShapeKind::RoundRect;
            cmd.pos = pos;
            cmd.mat = mat;
            cmd.normalTex = normalTex;

            // 将图标纹理作为材质纹理层叠加
            if (iconTex)
            {
                cmd.mat.texture.texID = iconTex;
                cmd.mat.texture.opacity = 1.f;
            }
            s_Cmds.push_back(cmd);
        }
    };

    // ================================================================
    //  § 13  Frame Lifecycle API
    // ================================================================

    /// 初始化渲染器 (必须在使用任何形状前调用一次)
    inline void Init() { Renderer::Get().Init(); }

    /// 每帧开始调用, 清空渲染命令列表
    inline void BeginFrame() { s_Cmds.clear(); }

    /// 在 ImGui::Render() 之后调用, 将所有玻璃元素提交至 OpenGL
    /// @param screenW/screenH  帧缓冲区像素尺寸
    inline void EndFrame(float screenW, float screenH)
    {
        Renderer::Get().RenderAll(screenW, screenH);
    }

    /// 设置屏幕捕获纹理 (每帧在 BeginFrame 前更新)
    inline void SetScreenTexture(GLuint tex) { Renderer::Get().screenTex = tex; }

    /// 设置环境亮度 (0=暗, 1=亮), 影响 Fresnel 高光颜色
    inline void SetEnvLuma(float luma) { Renderer::Get().envLuma = luma; }

    // ================================================================
    //  § 14  预设材质工厂 (便捷函数)
    // ================================================================
    namespace Preset
    {

        /// 标准毛玻璃 (蓝白色调)
        inline glass_material Frosted()
        {
            glass_material m;
            m.color.baseColor = {0.9f, 0.95f, 1.f, 1.f};
            m.color.tintStrength = 0.08f;
            m.blur.edgeBlur = 10.f;
            m.blur.startMargin = 8.f;
            m.refraction.refractIndex = 0.10f;
            m.refraction.flatTopRange = 0.4f;
            m.shadow.opacity = 0.3f;
            return m;
        }

        /// 液态玻璃 (高折射 + 轻微色散)
        inline glass_material Liquid()
        {
            glass_material m;
            m.blur.centerBlur = 2.f;
            m.blur.edgeBlur = 12.f;
            m.blur.startMargin = 4.f;
            m.refraction.refractIndex = 0.18f;
            m.refraction.normalTransition = 8.f;
            m.refraction.flatTopRange = 0.25f;
            m.refraction.dispersionCoeff = 0.02f;
            m.shadow.size = 14.f;
            m.shadow.blur = 16.f;
            m.shadow.opacity = 0.4f;
            return m;
        }

        /// 发光玻璃 (柔和白色发光)
        inline glass_material Glow(ImVec4 color = {1.f, 1.f, 1.f, 1.f}, float intensity = 0.3f)
        {
            glass_material m = Frosted();
            m.light.emitColor = color;
            m.light.intensity = intensity;
            return m;
        }

    } // namespace Preset

} // namespace LiquidGlassGround
