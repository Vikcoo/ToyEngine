// ToyEngine Core Module
// 整数向量和矩形类型 - 用于像素坐标、纹理尺寸、视口定义等

#pragma once

#include "MathTypes.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace TE {

// 前向声明
struct Rect;
struct IntRect;

// ==================== IntVector2 ====================

/// <summary>
/// 二维整数向量 - 用于像素坐标、纹理尺寸等
/// </summary>
struct [[nodiscard]] IntVector2
{
    int32_t X, Y;

    // 构造函数
    IntVector2() : X(0), Y(0) {}
    IntVector2(int32_t x, int32_t y) : X(x), Y(y) {}
    explicit IntVector2(int32_t scalar) : X(scalar), Y(scalar) {}

    // 常量
    static const IntVector2 Zero;
    static const IntVector2 One;

    // 运算符重载
    IntVector2 operator+(const IntVector2& other) const { return {X + other.X, Y + other.Y}; }
    IntVector2 operator-(const IntVector2& other) const { return {X - other.X, Y - other.Y}; }
    IntVector2 operator*(const IntVector2& other) const { return {X * other.X, Y * other.Y}; }
    IntVector2 operator*(int32_t scalar) const { return {X * scalar, Y * scalar}; }
    IntVector2 operator/(int32_t scalar) const { return {X / scalar, Y / scalar}; }
    IntVector2 operator-() const { return {-X, -Y}; }

    IntVector2& operator+=(const IntVector2& other) { X += other.X; Y += other.Y; return *this; }
    IntVector2& operator-=(const IntVector2& other) { X -= other.X; Y -= other.Y; return *this; }
    IntVector2& operator*=(int32_t scalar) { X *= scalar; Y *= scalar; return *this; }
    IntVector2& operator/=(int32_t scalar) { X /= scalar; Y /= scalar; return *this; }

    bool operator==(const IntVector2& other) const { return X == other.X && Y == other.Y; }
    bool operator!=(const IntVector2& other) const { return !(*this == other); }

    /// <summary>
    /// 转换为浮点向量
    /// </summary>
    Vector2 ToFloat() const { return {static_cast<float>(X), static_cast<float>(Y)}; }

    /// <summary>
    /// 从浮点向量转换（截断取整）
    /// </summary>
    static IntVector2 FromFloat(const Vector2& v)
    {
        return {static_cast<int32_t>(v.X), static_cast<int32_t>(v.Y)};
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static IntVector2 Min(const IntVector2& a, const IntVector2& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static IntVector2 Max(const IntVector2& a, const IntVector2& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y)};
    }
};

// 标量乘法（左操作数）
inline IntVector2 operator*(int32_t scalar, const IntVector2& vec) { return vec * scalar; }

// ==================== IntVector3 ====================

/// <summary>
/// 三维整数向量 - 用于网格索引、3D 纹理坐标等
/// </summary>
struct IntVector3
{
    int32_t X, Y, Z;

    // 构造函数
    IntVector3() : X(0), Y(0), Z(0) {}
    IntVector3(int32_t x, int32_t y, int32_t z) : X(x), Y(y), Z(z) {}
    explicit IntVector3(int32_t scalar) : X(scalar), Y(scalar), Z(scalar) {}

    // 常量
    static const IntVector3 Zero;
    static const IntVector3 One;

    // 运算符重载
    IntVector3 operator+(const IntVector3& other) const { return {X + other.X, Y + other.Y, Z + other.Z}; }
    IntVector3 operator-(const IntVector3& other) const { return {X - other.X, Y - other.Y, Z - other.Z}; }
    IntVector3 operator*(const IntVector3& other) const { return {X * other.X, Y * other.Y, Z * other.Z}; }
    IntVector3 operator*(int32_t scalar) const { return {X * scalar, Y * scalar, Z * scalar}; }
    IntVector3 operator/(int32_t scalar) const { return {X / scalar, Y / scalar, Z / scalar}; }
    IntVector3 operator-() const { return {-X, -Y, -Z}; }

    IntVector3& operator+=(const IntVector3& other) { X += other.X; Y += other.Y; Z += other.Z; return *this; }
    IntVector3& operator-=(const IntVector3& other) { X -= other.X; Y -= other.Y; Z -= other.Z; return *this; }
    IntVector3& operator*=(int32_t scalar) { X *= scalar; Y *= scalar; Z *= scalar; return *this; }
    IntVector3& operator/=(int32_t scalar) { X /= scalar; Y /= scalar; Z /= scalar; return *this; }

    bool operator==(const IntVector3& other) const { return X == other.X && Y == other.Y && Z == other.Z; }
    bool operator!=(const IntVector3& other) const { return !(*this == other); }

    /// <summary>
    /// 转换为浮点向量
    /// </summary>
    Vector3 ToFloat() const { return {static_cast<float>(X), static_cast<float>(Y), static_cast<float>(Z)}; }

    /// <summary>
    /// 从浮点向量转换（截断取整）
    /// </summary>
    static IntVector3 FromFloat(const Vector3& v)
    {
        return {static_cast<int32_t>(v.X), static_cast<int32_t>(v.Y), static_cast<int32_t>(v.Z)};
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static IntVector3 Min(const IntVector3& a, const IntVector3& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static IntVector3 Max(const IntVector3& a, const IntVector3& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z)};
    }
};

// 标量乘法（左操作数）
inline IntVector3 operator*(int32_t scalar, const IntVector3& vec) { return vec * scalar; }

// ==================== Rect ====================

/// <summary>
/// 浮点 2D 矩形 - 用于视口定义、UV 裁剪等
/// 存储格式：(X, Y, Width, Height)，与 Vulkan/D3D viewport 一致
/// </summary>
struct [[nodiscard]] Rect
{
    float X, Y, Width, Height;

    Rect() : X(0.0f), Y(0.0f), Width(0.0f), Height(0.0f) {}
    Rect(float x, float y, float width, float height)
        : X(x), Y(y), Width(width), Height(height)
    {}

    // 常量
    static const Rect Zero;
    static const Rect Unit;  // (0, 0, 1, 1) — 归一化矩形

    /// <summary>
    /// 从最小/最大点创建
    /// </summary>
    static Rect FromMinMax(float minX, float minY, float maxX, float maxY)
    {
        return {minX, minY, maxX - minX, maxY - minY};
    }

    /// <summary>
    /// 从中心和大小创建
    /// </summary>
    static Rect FromCenterSize(const Vector2& center, const Vector2& size)
    {
        return {center.X - size.X * 0.5f, center.Y - size.Y * 0.5f, size.X, size.Y};
    }

    // 属性
    float GetLeft() const { return X; }
    float GetTop() const { return Y; }
    float GetRight() const { return X + Width; }
    float GetBottom() const { return Y + Height; }

    Vector2 GetMin() const { return {X, Y}; }
    Vector2 GetMax() const { return {X + Width, Y + Height}; }
    Vector2 GetCenter() const { return {X + Width * 0.5f, Y + Height * 0.5f}; }
    Vector2 GetSize() const { return {Width, Height}; }
    Vector2 GetExtents() const { return {Width * 0.5f, Height * 0.5f}; }
    float GetArea() const { return Width * Height; }

    /// <summary>
    /// 是否包含点
    /// </summary>
    bool Contains(const Vector2& point) const
    {
        return point.X >= X && point.X <= X + Width &&
               point.Y >= Y && point.Y <= Y + Height;
    }

    /// <summary>
    /// 是否完全包含另一个矩形
    /// </summary>
    bool Contains(const Rect& other) const
    {
        return other.X >= X && other.X + other.Width <= X + Width &&
               other.Y >= Y && other.Y + other.Height <= Y + Height;
    }

    /// <summary>
    /// 是否与另一个矩形相交
    /// </summary>
    bool Intersects(const Rect& other) const
    {
        return X < other.X + other.Width && X + Width > other.X &&
               Y < other.Y + other.Height && Y + Height > other.Y;
    }

    /// <summary>
    /// 获取两个矩形的交集
    /// </summary>
    Rect Intersection(const Rect& other) const
    {
        float left = std::max(X, other.X);
        float top = std::max(Y, other.Y);
        float right = std::min(X + Width, other.X + other.Width);
        float bottom = std::min(Y + Height, other.Y + other.Height);

        if (right <= left || bottom <= top)
            return Rect::Zero;
        return {left, top, right - left, bottom - top};
    }

    /// <summary>
    /// 获取包含两个矩形的最小矩形
    /// </summary>
    Rect Union(const Rect& other) const
    {
        float left = std::min(X, other.X);
        float top = std::min(Y, other.Y);
        float right = std::max(X + Width, other.X + other.Width);
        float bottom = std::max(Y + Height, other.Y + other.Height);
        return {left, top, right - left, bottom - top};
    }

    /// <summary>
    /// 膨胀/收缩矩形
    /// </summary>
    Rect Inflate(float amount) const
    {
        return {X - amount, Y - amount, Width + amount * 2.0f, Height + amount * 2.0f};
    }

    bool operator==(const Rect& other) const
    {
        return X == other.X && Y == other.Y && Width == other.Width && Height == other.Height;
    }
    bool operator!=(const Rect& other) const { return !(*this == other); }
};

// ==================== IntRect ====================

/// <summary>
/// 整数 2D 矩形 - 用于像素区域、纹理裁剪、视口等
/// </summary>
struct [[nodiscard]] IntRect
{
    int32_t X, Y, Width, Height;

    IntRect() : X(0), Y(0), Width(0), Height(0) {}
    IntRect(int32_t x, int32_t y, int32_t width, int32_t height)
        : X(x), Y(y), Width(width), Height(height)
    {}

    // 常量
    static const IntRect Zero;

    /// <summary>
    /// 从最小/最大点创建
    /// </summary>
    static IntRect FromMinMax(int32_t minX, int32_t minY, int32_t maxX, int32_t maxY)
    {
        return {minX, minY, maxX - minX, maxY - minY};
    }

    // 属性
    int32_t GetLeft() const { return X; }
    int32_t GetTop() const { return Y; }
    int32_t GetRight() const { return X + Width; }
    int32_t GetBottom() const { return Y + Height; }

    IntVector2 GetMin() const { return {X, Y}; }
    IntVector2 GetMax() const { return {X + Width, Y + Height}; }
    IntVector2 GetSize() const { return {Width, Height}; }
    int32_t GetArea() const { return Width * Height; }

    /// <summary>
    /// 是否包含点
    /// </summary>
    bool Contains(const IntVector2& point) const
    {
        return point.X >= X && point.X < X + Width &&
               point.Y >= Y && point.Y < Y + Height;
    }

    /// <summary>
    /// 是否与另一个矩形相交
    /// </summary>
    bool Intersects(const IntRect& other) const
    {
        return X < other.X + other.Width && X + Width > other.X &&
               Y < other.Y + other.Height && Y + Height > other.Y;
    }

    /// <summary>
    /// 获取两个矩形的交集
    /// </summary>
    IntRect Intersection(const IntRect& other) const
    {
        int32_t left = std::max(X, other.X);
        int32_t top = std::max(Y, other.Y);
        int32_t right = std::min(X + Width, other.X + other.Width);
        int32_t bottom = std::min(Y + Height, other.Y + other.Height);

        if (right <= left || bottom <= top)
            return IntRect::Zero;
        return {left, top, right - left, bottom - top};
    }

    /// <summary>
    /// 转换为浮点矩形
    /// </summary>
    Rect ToFloat() const
    {
        return {static_cast<float>(X), static_cast<float>(Y),
                static_cast<float>(Width), static_cast<float>(Height)};
    }

    /// <summary>
    /// 从浮点矩形转换（截断取整）
    /// </summary>
    static IntRect FromFloat(const Rect& r)
    {
        return {static_cast<int32_t>(r.X), static_cast<int32_t>(r.Y),
                static_cast<int32_t>(r.Width), static_cast<int32_t>(r.Height)};
    }

    bool operator==(const IntRect& other) const
    {
        return X == other.X && Y == other.Y && Width == other.Width && Height == other.Height;
    }
    bool operator!=(const IntRect& other) const { return !(*this == other); }
};

} // namespace TE
