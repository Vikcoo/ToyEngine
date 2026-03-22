// ToyEngine Core Module
// 颜色类 - RGBA 表示和颜色操作

#pragma once

#include "MathTypes.h"
#include "MathUtils.h"
#include <cstdint>

namespace TE {

/// <summary>
/// 颜色类 - 使用 float 分量表示 RGBA
/// 范围：0.0 ~ 1.0（可超出范围用于 HDR）
/// </summary>
class Color
{
public:
    float R, G, B, A;

    // ==================== 构造函数 ====================

    /// <summary>
    /// 默认构造函数 - 透明黑色
    /// </summary>
    Color() : R(0.0f), G(0.0f), B(0.0f), A(0.0f) {}

    /// <summary>
    /// RGBA 构造函数
    /// </summary>
    Color(float r, float g, float b, float a = 1.0f) : R(r), G(g), B(b), A(a) {}

    /// <summary>
    /// 灰度构造函数
    /// </summary>
    explicit Color(float grayscale, float a = 1.0f) : R(grayscale), G(grayscale), B(grayscale), A(a) {}

    /// <summary>
    /// 从 Vector4 构造（RGBA 对应 XYZW）
    /// </summary>
    explicit Color(const Vector4& vec) : R(vec.X), G(vec.Y), B(vec.Z), A(vec.W) {}

    /// <summary>
    /// 从 Vector3 构造（RGB，Alpha 设为 1）
    /// </summary>
    explicit Color(const Vector3& vec) : R(vec.X), G(vec.Y), B(vec.Z), A(1.0f) {}

    /// <summary>
    /// 从 0-255 整数构造
    /// </summary>
    Color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
        : R(static_cast<float>(r) / 255.0f)
        , G(static_cast<float>(g) / 255.0f)
        , B(static_cast<float>(b) / 255.0f)
        , A(static_cast<float>(a) / 255.0f)
    {}

    // ==================== 转换 ====================

    /// <summary>
    /// 转换为 Vector4（RGBA 对应 XYZW）
    /// </summary>
    Vector4 ToVector4() const { return Vector4(R, G, B, A); }

    /// <summary>
    /// 从 Vector4 创建
    /// </summary>
    static Color FromVector4(const Vector4& vec) { return Color(vec); }

    /// <summary>
    /// 获取 RGB 部分作为 Vector3
    /// </summary>
    Vector3 ToVector3() const { return Vector3(R, G, B); }

    /// <summary>
    /// 获取 32 位整数表示（ARGB，用于 DirectX 等）
    /// </summary>
    uint32_t ToPackedARGB() const
    {
        uint8_t r = static_cast<uint8_t>(Math::Clamp(R, 0.0f, 1.0f) * 255.0f);
        uint8_t g = static_cast<uint8_t>(Math::Clamp(G, 0.0f, 1.0f) * 255.0f);
        uint8_t b = static_cast<uint8_t>(Math::Clamp(B, 0.0f, 1.0f) * 255.0f);
        uint8_t a = static_cast<uint8_t>(Math::Clamp(A, 0.0f, 1.0f) * 255.0f);
        return (a << 24) | (r << 16) | (g << 8) | b;
    }

    /// <summary>
    /// 获取 32 位整数表示（RGBA，用于 OpenGL 等）
    /// </summary>
    uint32_t ToPackedRGBA() const
    {
        uint8_t r = static_cast<uint8_t>(Math::Clamp(R, 0.0f, 1.0f) * 255.0f);
        uint8_t g = static_cast<uint8_t>(Math::Clamp(G, 0.0f, 1.0f) * 255.0f);
        uint8_t b = static_cast<uint8_t>(Math::Clamp(B, 0.0f, 1.0f) * 255.0f);
        uint8_t a = static_cast<uint8_t>(Math::Clamp(A, 0.0f, 1.0f) * 255.0f);
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

    /// <summary>
    /// 从 32 位整数创建（RGBA）
    /// </summary>
    static Color FromPackedRGBA(uint32_t rgba)
    {
        return Color(
            ((rgba >> 24) & 0xFF) / 255.0f,
            ((rgba >> 16) & 0xFF) / 255.0f,
            ((rgba >> 8) & 0xFF) / 255.0f,
            (rgba & 0xFF) / 255.0f
        );
    }

    // ==================== 运算符 ====================

    Color operator+(const Color& other) const { return Color(R + other.R, G + other.G, B + other.B, A + other.A); }
    Color operator-(const Color& other) const { return Color(R - other.R, G - other.G, B - other.B, A - other.A); }
    Color operator*(const Color& other) const { return Color(R * other.R, G * other.G, B * other.B, A * other.A); }
    Color operator*(float scalar) const { return Color(R * scalar, G * scalar, B * scalar, A * scalar); }
    Color operator/(float scalar) const { return Color(R / scalar, G / scalar, B / scalar, A / scalar); }
    Color operator-() const { return Color(-R, -G, -B, -A); }

    Color& operator+=(const Color& other) { R += other.R; G += other.G; B += other.B; A += other.A; return *this; }
    Color& operator-=(const Color& other) { R -= other.R; G -= other.G; B -= other.B; A -= other.A; return *this; }
    Color& operator*=(const Color& other) { R *= other.R; G *= other.G; B *= other.B; A *= other.A; return *this; }
    Color& operator*=(float scalar) { R *= scalar; G *= scalar; B *= scalar; A *= scalar; return *this; }
    Color& operator/=(float scalar) { R /= scalar; G /= scalar; B /= scalar; A /= scalar; return *this; }

    bool operator==(const Color& other) const { return R == other.R && G == other.G && B == other.B && A == other.A; }
    bool operator!=(const Color& other) const { return !(*this == other); }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// </summary>
    bool Equals(const Color& other, float epsilon = 1e-6f) const
    {
        return std::abs(R - other.R) <= epsilon &&
               std::abs(G - other.G) <= epsilon &&
               std::abs(B - other.B) <= epsilon &&
               std::abs(A - other.A) <= epsilon;
    }

    // ==================== 预设颜色 ====================

    static const Color White;       // (1, 1, 1, 1)
    static const Color Black;       // (0, 0, 0, 1)
    static const Color Red;         // (1, 0, 0, 1)
    static const Color Green;       // (0, 1, 0, 1)
    static const Color Blue;        // (0, 0, 1, 1)
    static const Color Yellow;      // (1, 1, 0, 1)
    static const Color Cyan;        // (0, 1, 1, 1)
    static const Color Magenta;     // (1, 0, 1, 1)
    static const Color Gray;        // (0.5, 0.5, 0.5, 1)
    static const Color LightGray;   // (0.75, 0.75, 0.75, 1)
    static const Color DarkGray;    // (0.25, 0.25, 0.25, 1)
    static const Color Orange;      // (1, 0.5, 0, 1)
    static const Color Purple;      // (0.5, 0, 0.5, 1)
    static const Color Brown;       // (0.6, 0.3, 0.1, 1)
    static const Color Transparent; // (0, 0, 0, 0)

    // ==================== 颜色操作 ====================

    /// <summary>
    /// 限制各分量在 0~1 范围
    /// </summary>
    Color Clamp01() const
    {
        return Color(
            Math::Clamp(R, 0.0f, 1.0f),
            Math::Clamp(G, 0.0f, 1.0f),
            Math::Clamp(B, 0.0f, 1.0f),
            Math::Clamp(A, 0.0f, 1.0f)
        );
    }

    /// <summary>
    /// 伽马校正（线性到 sRGB）
    /// </summary>
    Color ToSRGB() const
    {
        auto gammaCorrect = [](float c)
        {
            return (c > 0.0031308f) ? (1.055f * Math::Pow(c, 1.0f / 2.4f) - 0.055f) : (12.92f * c);
        };
        return Color(gammaCorrect(R), gammaCorrect(G), gammaCorrect(B), A);
    }

    /// <summary>
    /// 逆伽马校正（sRGB 到线性）
    /// </summary>
    Color ToLinear() const
    {
        auto gammaExpand = [](float c)
        {
            return (c > 0.04045f) ? Math::Pow((c + 0.055f) / 1.055f, 2.4f) : (c / 12.92f);
        };
        return Color(gammaExpand(R), gammaExpand(G), gammaExpand(B), A);
    }

    /// <summary>
    /// 亮度（感知亮度，Rec. 601）
    /// </summary>
    float Luminance() const
    {
        return 0.299f * R + 0.587f * G + 0.114f * B;
    }

    /// <summary>
    /// 去饱和（转为灰度）
    /// </summary>
    Color Grayscale() const
    {
        float lum = Luminance();
        return Color(lum, lum, lum, A);
    }

    /// <summary>
    /// 反色
    /// </summary>
    Color Invert() const
    {
        return Color(1.0f - R, 1.0f - G, 1.0f - B, A);
    }

    /// <summary>
    /// 反色（包括 Alpha）
    /// </summary>
    Color InvertRGBA() const
    {
        return Color(1.0f - R, 1.0f - G, 1.0f - B, 1.0f - A);
    }

    /// <summary>
    /// 透明度预乘（用于渲染混合）
    /// </summary>
    Color PremultiplyAlpha() const
    {
        return Color(R * A, G * A, B * A, A);
    }

    // ==================== HSV 转换 ====================

    /// <summary>
    /// 从 HSV 创建颜色
    /// H: 0~360 度, S: 0~1, V: 0~1
    /// </summary>
    static Color FromHSV(float h, float s, float v, float a = 1.0f);

    /// <summary>
    /// 转换为 HSV
    /// 返回 Vector3(h, s, v)，h 为 0~360 度，s 和 v 为 0~1
    /// </summary>
    Vector3 ToHSV() const;

    /// <summary>
    /// 色相偏移
    /// </summary>
    Color ShiftHue(float degrees) const
    {
        Vector3 hsv = ToHSV();
        hsv.X = Math::Repeat(hsv.X + degrees, 360.0f);
        return FromHSV(hsv.X, hsv.Y, hsv.Z, A);
    }

    /// <summary>
    /// 调整饱和度
    /// </summary>
    Color Saturate(float delta) const
    {
        Vector3 hsv = ToHSV();
        hsv.Y = Math::Clamp(hsv.Y + delta, 0.0f, 1.0f);
        return FromHSV(hsv.X, hsv.Y, hsv.Z, A);
    }

    /// <summary>
    /// 调整明度
    /// </summary>
    Color Brighten(float delta) const
    {
        Vector3 hsv = ToHSV();
        hsv.Z = Math::Clamp(hsv.Z + delta, 0.0f, 1.0f);
        return FromHSV(hsv.X, hsv.Y, hsv.Z, A);
    }

    // ==================== 插值 ====================

    /// <summary>
    /// RGBA 线性插值
    /// </summary>
    static Color Lerp(const Color& a, const Color& b, float t)
    {
        return Color(
            Math::Lerp(a.R, b.R, t),
            Math::Lerp(a.G, b.G, t),
            Math::Lerp(a.B, b.B, t),
            Math::Lerp(a.A, b.A, t)
        );
    }

    /// <summary>
    /// RGB 线性插值，保持 Alpha
    /// </summary>
    static Color LerpRGB(const Color& a, const Color& b, float t)
    {
        return Color(
            Math::Lerp(a.R, b.R, t),
            Math::Lerp(a.G, b.G, t),
            Math::Lerp(a.B, b.B, t),
            a.A
        );
    }

    /// <summary>
    /// HSV 空间插值（色相选择最短路径）
    /// </summary>
    static Color LerpHSV(const Color& a, const Color& b, float t);

};

// 标量乘法（左操作数）
inline Color operator*(float scalar, const Color& color) { return color * scalar; }

} // namespace TE
