// ToyEngine Core Module
// 四元数类型定义

#pragma once

#include "Vector.h"

namespace TE {

struct Matrix3;
struct Matrix4;

// ==================== Quat（四元数） ====================
struct [[nodiscard]] Quat
{
    float X, Y, Z, W;  // W 是实部，X,Y,Z 是虚部

    // 构造函数
    Quat() : X(0.0f), Y(0.0f), Z(0.0f), W(1.0f) {}  // 单位四元数
    Quat(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

    // 从轴角构造
    Quat(const Vector3& axis, float angleRadians)
    {
        float halfAngle = angleRadians * 0.5f;
        float s = std::sin(halfAngle);
        Vector3 normalizedAxis = axis.Normalize();
        X = normalizedAxis.X * s;
        Y = normalizedAxis.Y * s;
        Z = normalizedAxis.Z * s;
        W = std::cos(halfAngle);
    }

    // 从欧拉角构造（yaw, pitch, roll）
    static Quat FromEuler(float yaw, float pitch, float roll);

    // 常量
    static const Quat Identity;

    // 运算符重载
    Quat operator+(const Quat& other) const { return {X + other.X, Y + other.Y, Z + other.Z, W + other.W}; }
    Quat operator-(const Quat& other) const { return {X - other.X, Y - other.Y, Z - other.Z, W - other.W}; }
    Quat operator-() const { return {-X, -Y, -Z, -W}; }
    Quat operator*(float scalar) const { return {X * scalar, Y * scalar, Z * scalar, W * scalar}; }
    Quat operator/(float scalar) const { return {X / scalar, Y / scalar, Z / scalar, W / scalar}; }

    Quat& operator+=(const Quat& other) { X += other.X; Y += other.Y; Z += other.Z; W += other.W; return *this; }
    Quat& operator-=(const Quat& other) { X -= other.X; Y -= other.Y; Z -= other.Z; W -= other.W; return *this; }
    Quat& operator*=(float scalar) { X *= scalar; Y *= scalar; Z *= scalar; W *= scalar; return *this; }
    Quat& operator/=(float scalar) { X /= scalar; Y /= scalar; Z /= scalar; W /= scalar; return *this; }

    // 四元数乘法（表示旋转的组合）
    Quat operator*(const Quat& other) const
    {
        return {
            W * other.X + X * other.W + Y * other.Z - Z * other.Y,
            W * other.Y - X * other.Z + Y * other.W + Z * other.X,
            W * other.Z + X * other.Y - Y * other.X + Z * other.W,
            W * other.W - X * other.X - Y * other.Y - Z * other.Z
        };
    }

    Quat& operator*=(const Quat& other) { *this = *this * other; return *this; }

    // 旋转一个向量
    Vector3 operator*(const Vector3& vec) const
    {
        // q * v * q^-1
        Quat vecQuat(vec.X, vec.Y, vec.Z, 0.0f);
        Quat result = *this * vecQuat * Inverse();
        return {result.X, result.Y, result.Z};
    }

    bool operator==(const Quat& other) const { return X == other.X && Y == other.Y && Z == other.Z && W == other.W; }
    bool operator!=(const Quat& other) const { return !(*this == other); }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// 注意：四元数 q 和 -q 表示相同旋转，此方法会同时检查两种情况
    /// </summary>
    [[nodiscard]] bool Equals(const Quat& other, float epsilon = 1e-6f) const
    {
        // q 和 -q 表示相同旋转
        bool direct = std::abs(X - other.X) <= epsilon &&
                       std::abs(Y - other.Y) <= epsilon &&
                       std::abs(Z - other.Z) <= epsilon &&
                       std::abs(W - other.W) <= epsilon;
        if (direct) return true;
        bool negated = std::abs(X + other.X) <= epsilon &&
                        std::abs(Y + other.Y) <= epsilon &&
                        std::abs(Z + other.Z) <= epsilon &&
                        std::abs(W + other.W) <= epsilon;
        return negated;
    }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y + Z * Z + W * W); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y + Z * Z + W * W; }

    Quat Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Quat{X / len, Y / len, Z / len, W / len} : Quat::Identity;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; Z /= len; W /= len; }
    }

    Quat Conjugate() const { return {-X, -Y, -Z, W}; }

    Quat Inverse() const
    {
        float lenSq = LengthSquared();
        return lenSq > 0.0f ? Conjugate() / lenSq : Quat::Identity;
    }

    // 转换为旋转矩阵
    Matrix4 ToMatrix4() const;
    Matrix3 ToMatrix3() const;

    // 转换为欧拉角（返回 yaw, pitch, roll）
    Vector3 ToEulerAngles() const;

    // 线性插值（非归一化，快速但不精确）
    static Quat Lerp(const Quat& a, const Quat& b, float t)
    {
        return (a * (1.0f - t) + b * t).Normalize();
    }

    // 球面线性插值（归一化，精确）
    static Quat Slerp(const Quat& a, const Quat& b, float t);

    // 获取旋转轴和角度
    void GetAxisAngle(Vector3& outAxis, float& outAngleRadians) const;
};

// 标量乘法（左操作数）
inline Quat operator*(float scalar, const Quat& q) { return q * scalar; }

} // namespace TE
