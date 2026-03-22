// ToyEngine Core Module
// 数学工具函数

#pragma once

#include "MathTypes.h"
#include <cmath>
#include <algorithm>


namespace TE::Math {

// ==================== 常量 ====================

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = 0.5f * PI;
constexpr float EPSILON = 1e-6f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// ==================== 基础函数 ====================

/// <summary>
/// 绝对值
/// </summary>
template<typename T>
inline T Abs(T value)
{
    return std::abs(value);
}

/// <summary>
/// 最小值
/// </summary>
template<typename T>
inline T Min(T a, T b)
{
    return std::min(a, b);
}

/// <summary>
/// 最大值
/// </summary>
template<typename T>
inline T Max(T a, T b)
{
    return std::max(a, b);
}

/// <summary>
/// 三个数的最小值
/// </summary>
template<typename T>
inline T Min(T a, T b, T c)
{
    return std::min(std::min(a, b), c);
}

/// <summary>
/// 三个数的最大值
/// </summary>
template<typename T>
inline T Max(T a, T b, T c)
{
    return std::max(std::max(a, b), c);
}

/// <summary>
/// 限制在范围内（包含边界）
/// </summary>
template<typename T>
inline T Clamp(T value, T min, T max)
{
    return std::max(min, std::min(value, max));
}

/// <summary>
/// 限制在 0~1 范围内
/// </summary>
template<typename T>
inline T Saturate(T value)
{
    return Clamp(value, T(0), T(1));
}

/// <summary>
/// 度转弧度
/// </summary>
inline float DegToRad(float degrees)
{
    return degrees * DEG_TO_RAD;
}

/// <summary>
/// 弧度转度
/// </summary>
inline float RadToDeg(float radians)
{
    return radians * RAD_TO_DEG;
}

// ==================== 指数和对数 ====================

/// <summary>
/// 平方根
/// </summary>
inline float Sqrt(float value)
{
    return std::sqrt(value);
}

/// <summary>
/// 平方
/// </summary>
inline float Square(float value)
{
    return value * value;
}

/// <summary>
/// 幂
/// </summary>
inline float Pow(float base, float exp)
{
    return std::pow(base, exp);
}

/// <summary>
/// 自然对数
/// </summary>
inline float Log(float value)
{
    return std::log(value);
}

/// <summary>
/// 以 10 为底的对数
/// </summary>
inline float Log10(float value)
{
    return std::log10(value);
}

/// <summary>
/// 指数函数 e^x
/// </summary>
inline float Exp(float value)
{
    return std::exp(value);
}

// ==================== 三角函数 ====================

inline float Sin(float radians) { return std::sin(radians); }
inline float Cos(float radians) { return std::cos(radians); }
inline float Tan(float radians) { return std::tan(radians); }

inline float Asin(float value) { return std::asin(Clamp(value, -1.0f, 1.0f)); }
inline float Acos(float value) { return std::acos(Clamp(value, -1.0f, 1.0f)); }
inline float Atan(float value) { return std::atan(value); }
inline float Atan2(float y, float x) { return std::atan2(y, x); }

/// <summary>
/// 角度的 sin（输入为度）
/// </summary>
inline float SinDeg(float degrees) { return Sin(DegToRad(degrees)); }

/// <summary>
/// 角度的 cos（输入为度）
/// </summary>
inline float CosDeg(float degrees) { return Cos(DegToRad(degrees)); }

/// <summary>
/// 角度的 tan（输入为度）
/// </summary>
inline float TanDeg(float degrees) { return Tan(DegToRad(degrees)); }

// ==================== 插值 ====================

/// <summary>
/// 线性插值
/// </summary>
inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

/// <summary>
/// 反向线性插值（获取 t 值）
/// 当 a == b 时返回 0 避免除零
/// </summary>
inline float InverseLerp(float a, float b, float value)
{
    float denom = b - a;
    if (std::abs(denom) < EPSILON)
        return 0.0f;
    return (value - a) / denom;
}

/// <summary>
/// 平滑步进（Hermite 插值）
/// 当 edge0 == edge1 时返回 0
/// </summary>
inline float SmoothStep(float edge0, float edge1, float value)
{
    float denom = edge1 - edge0;
    if (std::abs(denom) < EPSILON)
        return 0.0f;
    float t = Saturate((value - edge0) / denom);
    return t * t * (3.0f - 2.0f * t);
}

/// <summary>
/// 更平滑的步进（更高阶）
/// 当 edge0 == edge1 时返回 0
/// </summary>
inline float SmootherStep(float edge0, float edge1, float value)
{
    float denom = edge1 - edge0;
    if (std::abs(denom) < EPSILON)
        return 0.0f;
    float t = Saturate((value - edge0) / denom);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

/// <summary>
/// 移动到目标值（带速度限制）
/// </summary>
inline float MoveTowards(float current, float target, float maxDelta)
{
    if (std::abs(target - current) <= maxDelta)
        return target;
    return current + std::copysign(maxDelta, target - current);
}

/// <summary>
/// 角度移动到目标值（处理环绕）
/// </summary>
inline float MoveTowardsAngle(float current, float target, float maxDelta)
{
    float delta = target - current;
    // 归一化到 -180 ~ 180
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;

    if (std::abs(delta) <= maxDelta)
        return target;
    return current + std::copysign(maxDelta, delta);
}

// ==================== 舍入 ====================

/// <summary>
/// 向下取整
/// </summary>
inline float Floor(float value)
{
    return std::floor(value);
}

/// <summary>
/// 向上取整
/// </summary>
inline float Ceil(float value)
{
    return std::ceil(value);
}

/// <summary>
/// 四舍五入
/// </summary>
inline float Round(float value)
{
    return std::round(value);
}

/// <summary>
/// 截断小数部分
/// </summary>
inline float Trunc(float value)
{
    return std::trunc(value);
}

/// <summary>
/// 小数部分
/// </summary>
inline float Frac(float value)
{
    return value - Floor(value);
}

/// <summary>
/// 重复值在范围内（模运算，处理负数）
/// </summary>
inline float Repeat(float value, float length)
{
    return Clamp(value - Floor(value / length) * length, 0.0f, length);
}

/// <summary>
/// 乒乓震荡（三角波）
/// </summary>
inline float PingPong(float value, float length)
{
    float t = Repeat(value, length * 2.0f);
    return length - std::abs(t - length);
}

// ==================== 比较 ====================

/// <summary>
/// 接近相等（考虑浮点误差）
/// </summary>
inline bool Approximately(float a, float b, float epsilon = EPSILON)
{
    return std::abs(b - a) < std::max(EPSILON * std::max(std::abs(a), std::abs(b)), epsilon);
}

/// <summary>
/// 符号函数
/// </summary>
inline float Sign(float value)
{
    return (value > 0.0f) ? 1.0f : ((value < 0.0f) ? -1.0f : 0.0f);
}

/// <summary>
/// 符号函数（0 返回 1）
/// </summary>
inline float SignNoZero(float value)
{
    return value >= 0.0f ? 1.0f : -1.0f;
}

/// <summary>
/// 取正数部分
/// </summary>
template<typename T>
inline T MaxZero(T value)
{
    return std::max(T(0), value);
}

// ==================== 向量工具 ====================

/// <summary>
/// 向量线性插值
/// </summary>
inline Vector2 Lerp(const Vector2& a, const Vector2& b, float t)
{
    return Vector2::Lerp(a, b, t);
}

inline Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
{
    return Vector3::Lerp(a, b, t);
}

inline Vector4 Lerp(const Vector4& a, const Vector4& b, float t)
{
    return Vector4::Lerp(a, b, t);
}

/// <summary>
/// 球形线性插值（向量）
/// </summary>
inline Vector3 Slerp(const Vector3& a, const Vector3& b, float t)
{
    float dot = Vector3::Dot(a, b);
    dot = Clamp(dot, -1.0f, 1.0f);

    if (dot > 0.9995f)
    {
        // 向量几乎平行，使用线性插值
        return Vector3::Lerp(a, b, t).Normalize();
    }

    float theta = std::acos(dot) * t;
    Vector3 relativeVec = (b - a * dot).Normalize();
    return a * std::cos(theta) + relativeVec * std::sin(theta);
}

// ==================== 范围映射 ====================

/// <summary>
/// 将一个范围的值映射到另一个范围
/// 当 fromMin == fromMax 时返回 toMin 避免除零
/// </summary>
inline float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
{
    float denom = fromMax - fromMin;
    if (std::abs(denom) < EPSILON)
        return toMin;
    float t = (value - fromMin) / denom;
    return toMin + t * (toMax - toMin);
}

/// <summary>
/// 限制在范围内（如果超出则返回边界值）
/// </summary>
inline float Clamp01(float value)
{
    return Clamp(value, 0.0f, 1.0f);
}

} // namespace TE::Math

