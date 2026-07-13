// ToyEngine Core Module
// 向量类型定义

#pragma once

#include <algorithm>
#include <cmath>

namespace TE {

// ==================== Vector2 ====================
struct [[nodiscard]] Vector2
{
    float X, Y;

    // 构造函数
    Vector2() : X(0.0f), Y(0.0f) {}
    Vector2(float x, float y) : X(x), Y(y) {}
    explicit Vector2(float scalar) : X(scalar), Y(scalar) {}

    // 常量
    static const Vector2 Zero;
    static const Vector2 One;
    static const Vector2 Right;
    static const Vector2 Up;

    // 运算符重载
    Vector2 operator+(const Vector2& other) const { return {X + other.X, Y + other.Y}; }
    Vector2 operator-(const Vector2& other) const { return {X - other.X, Y - other.Y}; }
    Vector2 operator*(const Vector2& other) const { return {X * other.X, Y * other.Y}; }
    Vector2 operator/(const Vector2& other) const { return {X / other.X, Y / other.Y}; }
    Vector2 operator*(float scalar) const { return {X * scalar, Y * scalar}; }
    Vector2 operator/(float scalar) const { return {X / scalar, Y / scalar}; }
    Vector2 operator-() const { return {-X, -Y}; }

    Vector2& operator+=(const Vector2& other) { X += other.X; Y += other.Y; return *this; }
    Vector2& operator-=(const Vector2& other) { X -= other.X; Y -= other.Y; return *this; }
    Vector2& operator*=(const Vector2& other) { X *= other.X; Y *= other.Y; return *this; }
    Vector2& operator/=(const Vector2& other) { X /= other.X; Y /= other.Y; return *this; }
    Vector2& operator*=(float scalar) { X *= scalar; Y *= scalar; return *this; }
    Vector2& operator/=(float scalar) { X /= scalar; Y /= scalar; return *this; }

    bool operator==(const Vector2& other) const { return X == other.X && Y == other.Y; }
    bool operator!=(const Vector2& other) const { return !(*this == other); }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y; }

    Vector2 Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Vector2{X / len, Y / len} : Vector2::Zero;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; }
    }

    static float Dot(const Vector2& a, const Vector2& b) { return a.X * b.X + a.Y * b.Y; }
    static float Distance(const Vector2& a, const Vector2& b) { return (a - b).Length(); }
    static float DistanceSquared(const Vector2& a, const Vector2& b) { return (a - b).LengthSquared(); }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// </summary>
    [[nodiscard]] bool Equals(const Vector2& other, float epsilon = 1e-6f) const
    {
        return std::abs(X - other.X) <= epsilon && std::abs(Y - other.Y) <= epsilon;
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static Vector2 Min(const Vector2& a, const Vector2& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static Vector2 Max(const Vector2& a, const Vector2& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y)};
    }

    // 线性插值
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)
    {
        return a + (b - a) * t;
    }
};

// 标量乘法（左操作数）
inline Vector2 operator*(float scalar, const Vector2& vec) { return vec * scalar; }

// ==================== Vector3 ====================
struct [[nodiscard]] Vector3
{
    float X, Y, Z;

    // 构造函数
    Vector3() : X(0.0f), Y(0.0f), Z(0.0f) {}
    Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}
    explicit Vector3(float scalar) : X(scalar), Y(scalar), Z(scalar) {}

    // 常量
    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 Right;
    static const Vector3 Up;
    static const Vector3 Forward;

    // 运算符重载
    Vector3 operator+(const Vector3& other) const { return {X + other.X, Y + other.Y, Z + other.Z}; }
    Vector3 operator-(const Vector3& other) const { return {X - other.X, Y - other.Y, Z - other.Z}; }
    Vector3 operator*(const Vector3& other) const { return {X * other.X, Y * other.Y, Z * other.Z}; }
    Vector3 operator/(const Vector3& other) const { return {X / other.X, Y / other.Y, Z / other.Z}; }
    Vector3 operator*(float scalar) const { return {X * scalar, Y * scalar, Z * scalar}; }
    Vector3 operator/(float scalar) const { return {X / scalar, Y / scalar, Z / scalar}; }
    Vector3 operator-() const { return {-X, -Y, -Z}; }

    Vector3& operator+=(const Vector3& other) { X += other.X; Y += other.Y; Z += other.Z; return *this; }
    Vector3& operator-=(const Vector3& other) { X -= other.X; Y -= other.Y; Z -= other.Z; return *this; }
    Vector3& operator*=(const Vector3& other) { X *= other.X; Y *= other.Y; Z *= other.Z; return *this; }
    Vector3& operator/=(const Vector3& other) { X /= other.X; Y /= other.Y; Z /= other.Z; return *this; }
    Vector3& operator*=(float scalar) { X *= scalar; Y *= scalar; Z *= scalar; return *this; }
    Vector3& operator/=(float scalar) { X /= scalar; Y /= scalar; Z /= scalar; return *this; }

    bool operator==(const Vector3& other) const { return X == other.X && Y == other.Y && Z == other.Z; }
    bool operator!=(const Vector3& other) const { return !(*this == other); }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y + Z * Z; }

    Vector3 Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Vector3{X / len, Y / len, Z / len} : Vector3::Zero;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; Z /= len; }
    }

    static float Dot(const Vector3& a, const Vector3& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }
    static Vector3 Cross(const Vector3& a, const Vector3& b)
    {
        return {
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X
        };
    }
    static float Distance(const Vector3& a, const Vector3& b) { return (a - b).Length(); }
    static float DistanceSquared(const Vector3& a, const Vector3& b) { return (a - b).LengthSquared(); }

    // 线性插值
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        return a + (b - a) * t;
    }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// </summary>
    [[nodiscard]] bool Equals(const Vector3& other, float epsilon = 1e-6f) const
    {
        return std::abs(X - other.X) <= epsilon &&
               std::abs(Y - other.Y) <= epsilon &&
               std::abs(Z - other.Z) <= epsilon;
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static Vector3 Min(const Vector3& a, const Vector3& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static Vector3 Max(const Vector3& a, const Vector3& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z)};
    }

    /// <summary>
    /// 计算两个向量之间的夹角（弧度）
    /// </summary>
    static float Angle(const Vector3& a, const Vector3& b)
    {
        float lenProduct = a.Length() * b.Length();
        if (lenProduct < 1e-6f)
            return 0.0f;
        float dot = Dot(a, b) / lenProduct;
        // Clamp 以避免 acos 参数超出 [-1, 1]
        dot = std::max(-1.0f, std::min(1.0f, dot));
        return std::acos(dot);
    }

    /// <summary>
    /// 限制向量长度不超过 maxLength
    /// </summary>
    Vector3 ClampLength(float maxLength) const
    {
        float lenSq = LengthSquared();
        if (lenSq > maxLength * maxLength)
        {
            float len = std::sqrt(lenSq);
            return *this * (maxLength / len);
        }
        return *this;
    }

    /// <summary>
    /// 从当前位置向目标移动，每次最多移动 maxDelta 距离
    /// </summary>
    static Vector3 MoveTowards(const Vector3& current, const Vector3& target, float maxDelta)
    {
        Vector3 diff = target - current;
        float dist = diff.Length();
        if (dist <= maxDelta || dist < 1e-6f)
            return target;
        return current + diff * (maxDelta / dist);
    }

    /// <summary>
    /// 弹簧阻尼插值（临界阻尼弹簧，常用于摄像机跟随）
    /// </summary>
    /// <param name="current">当前位置</param>
    /// <param name="target">目标位置</param>
    /// <param name="currentVelocity">当前速度（会被修改）</param>
    /// <param name="smoothTime">到达目标的大致时间</param>
    /// <param name="deltaTime">帧时间间隔</param>
    /// <param name="maxSpeed">最大速度限制</param>
    static Vector3 SmoothDamp(const Vector3& current, const Vector3& target, Vector3& currentVelocity,
                               float smoothTime, float deltaTime, float maxSpeed = 1e30f)
    {
        // 基于 Unity 的临界阻尼弹簧实现
        smoothTime = std::max(0.0001f, smoothTime);
        float omega = 2.0f / smoothTime;
        float x = omega * deltaTime;
        // 近似 exp(-x)
        float exp_factor = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

        Vector3 diff = current - target;
        Vector3 origTarget = target;

        // 限制最大位移
        float maxDist = maxSpeed * smoothTime;
        diff = diff.ClampLength(maxDist);
        Vector3 clampedTarget = current - diff;

        Vector3 temp = (currentVelocity + diff * omega) * deltaTime;
        currentVelocity = (currentVelocity - temp * omega) * exp_factor;
        Vector3 result = clampedTarget + (diff + temp) * exp_factor;

        // 防止超调
        if (Dot(origTarget - current, result - origTarget) > 0.0f)
        {
            result = origTarget;
            currentVelocity = (result - origTarget) * (1.0f / deltaTime);
        }

        return result;
    }

    //  todo 反射和投影
    Vector3 Reflect(const Vector3& normal) const
    {
        // 注意：使用 normal * scalar 避免依赖全局左乘运算符
        return *this - normal * (2.0f * Dot(*this, normal));
    }

    Vector3 Project(const Vector3& onto) const
    {
        float dot = Dot(*this, onto);
        float ontoLenSq = onto.LengthSquared();
        return ontoLenSq > 0.0f ? onto * (dot / ontoLenSq) : Vector3::Zero;
    }
};

// 标量乘法（左操作数）
inline Vector3 operator*(float scalar, const Vector3& vec) { return vec * scalar; }

// ==================== Vector4 ====================
struct [[nodiscard]] Vector4
{
    float X, Y, Z, W;

    // 构造函数
    Vector4() : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f) {}
    Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
    explicit Vector4(float scalar) : X(scalar), Y(scalar), Z(scalar), W(scalar) {}
    Vector4(const Vector3& xyz, float w) : X(xyz.X), Y(xyz.Y), Z(xyz.Z), W(w) {}

    // 获取 XYZ 部分作为 Vector3
    Vector3 GetXYZ() const { return {X, Y, Z}; }

    // 常量
    static const Vector4 Zero;
    static const Vector4 One;

    // 运算符重载
    Vector4 operator+(const Vector4& other) const { return {X + other.X, Y + other.Y, Z + other.Z, W + other.W}; }
    Vector4 operator-(const Vector4& other) const { return {X - other.X, Y - other.Y, Z - other.Z, W - other.W}; }
    Vector4 operator*(const Vector4& other) const { return {X * other.X, Y * other.Y, Z * other.Z, W * other.W}; }
    Vector4 operator/(const Vector4& other) const { return {X / other.X, Y / other.Y, Z / other.Z, W / other.W}; }
    Vector4 operator*(float scalar) const { return {X * scalar, Y * scalar, Z * scalar, W * scalar}; }
    Vector4 operator/(float scalar) const { return {X / scalar, Y / scalar, Z / scalar, W / scalar}; }
    Vector4 operator-() const { return {-X, -Y, -Z, -W}; }

    Vector4& operator+=(const Vector4& other) { X += other.X; Y += other.Y; Z += other.Z; W += other.W; return *this; }
    Vector4& operator-=(const Vector4& other) { X -= other.X; Y -= other.Y; Z -= other.Z; W -= other.W; return *this; }
    Vector4& operator*=(const Vector4& other) { X *= other.X; Y *= other.Y; Z *= other.Z; W *= other.W; return *this; }
    Vector4& operator/=(const Vector4& other) { X /= other.X; Y /= other.Y; Z /= other.Z; W /= other.W; return *this; }
    Vector4& operator*=(float scalar) { X *= scalar; Y *= scalar; Z *= scalar; W *= scalar; return *this; }
    Vector4& operator/=(float scalar) { X /= scalar; Y /= scalar; Z /= scalar; W /= scalar; return *this; }

    bool operator==(const Vector4& other) const { return X == other.X && Y == other.Y && Z == other.Z && W == other.W; }
    bool operator!=(const Vector4& other) const { return !(*this == other); }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y + Z * Z + W * W); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y + Z * Z + W * W; }

    Vector4 Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Vector4{X / len, Y / len, Z / len, W / len} : Vector4::Zero;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; Z /= len; W /= len; }
    }

    static float Dot(const Vector4& a, const Vector4& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W; }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// </summary>
    [[nodiscard]] bool Equals(const Vector4& other, float epsilon = 1e-6f) const
    {
        return std::abs(X - other.X) <= epsilon &&
               std::abs(Y - other.Y) <= epsilon &&
               std::abs(Z - other.Z) <= epsilon &&
               std::abs(W - other.W) <= epsilon;
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static Vector4 Min(const Vector4& a, const Vector4& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z), std::min(a.W, b.W)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static Vector4 Max(const Vector4& a, const Vector4& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z), std::max(a.W, b.W)};
    }

    // 线性插值
    static Vector4 Lerp(const Vector4& a, const Vector4& b, float t)
    {
        return a + (b - a) * t;
    }
};

// 标量乘法（左操作数）
inline Vector4 operator*(float scalar, const Vector4& vec) { return vec * scalar; }

} // namespace TE
