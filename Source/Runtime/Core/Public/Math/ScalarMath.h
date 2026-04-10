// ToyEngine Core Module
// 轻量标量数学工具函数

#pragma once

#include <algorithm>
#include <cmath>

namespace TE::Math {

// ==================== 常量 ====================

constexpr float PI = 3.14159265358979323846f;
constexpr float TWO_PI = 2.0f * PI;
constexpr float HALF_PI = 0.5f * PI;
constexpr float EPSILON = 1e-6f;
constexpr float DEG_TO_RAD = PI / 180.0f;
constexpr float RAD_TO_DEG = 180.0f / PI;

// ==================== 基础函数 ====================

template<typename T>
inline T Abs(T value)
{
    return std::abs(value);
}

template<typename T>
inline T Min(T a, T b)
{
    return std::min(a, b);
}

template<typename T>
inline T Max(T a, T b)
{
    return std::max(a, b);
}

template<typename T>
inline T Min(T a, T b, T c)
{
    return std::min(std::min(a, b), c);
}

template<typename T>
inline T Max(T a, T b, T c)
{
    return std::max(std::max(a, b), c);
}

template<typename T>
inline T Clamp(T value, T min, T max)
{
    return std::max(min, std::min(value, max));
}

template<typename T>
inline T Saturate(T value)
{
    return Clamp(value, T(0), T(1));
}

inline float DegToRad(float degrees)
{
    return degrees * DEG_TO_RAD;
}

inline float RadToDeg(float radians)
{
    return radians * RAD_TO_DEG;
}

// ==================== 指数和对数 ====================

inline float Sqrt(float value)
{
    return std::sqrt(value);
}

inline float Square(float value)
{
    return value * value;
}

inline float Pow(float base, float exp)
{
    return std::pow(base, exp);
}

inline float Log(float value)
{
    return std::log(value);
}

inline float Log10(float value)
{
    return std::log10(value);
}

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

inline float SinDeg(float degrees) { return Sin(DegToRad(degrees)); }
inline float CosDeg(float degrees) { return Cos(DegToRad(degrees)); }
inline float TanDeg(float degrees) { return Tan(DegToRad(degrees)); }

// ==================== 插值 ====================

inline float Lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

inline float InverseLerp(float a, float b, float value)
{
    float denom = b - a;
    if (std::abs(denom) < EPSILON)
        return 0.0f;
    return (value - a) / denom;
}

inline float SmoothStep(float edge0, float edge1, float value)
{
    float denom = edge1 - edge0;
    if (std::abs(denom) < EPSILON)
        return 0.0f;
    float t = Saturate((value - edge0) / denom);
    return t * t * (3.0f - 2.0f * t);
}

inline float SmootherStep(float edge0, float edge1, float value)
{
    float denom = edge1 - edge0;
    if (std::abs(denom) < EPSILON)
        return 0.0f;
    float t = Saturate((value - edge0) / denom);
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

inline float MoveTowards(float current, float target, float maxDelta)
{
    if (std::abs(target - current) <= maxDelta)
        return target;
    return current + std::copysign(maxDelta, target - current);
}

inline float MoveTowardsAngle(float current, float target, float maxDelta)
{
    float delta = target - current;
    while (delta > 180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;

    if (std::abs(delta) <= maxDelta)
        return target;
    return current + std::copysign(maxDelta, delta);
}

// ==================== 舍入 ====================

inline float Floor(float value)
{
    return std::floor(value);
}

inline float Ceil(float value)
{
    return std::ceil(value);
}

inline float Round(float value)
{
    return std::round(value);
}

inline float Trunc(float value)
{
    return std::trunc(value);
}

inline float Frac(float value)
{
    return value - Floor(value);
}

inline float Repeat(float value, float length)
{
    return Clamp(value - Floor(value / length) * length, 0.0f, length);
}

inline float PingPong(float value, float length)
{
    float t = Repeat(value, length * 2.0f);
    return length - std::abs(t - length);
}

// ==================== 比较 ====================

inline bool Approximately(float a, float b, float epsilon = EPSILON)
{
    return std::abs(b - a) < std::max(EPSILON * std::max(std::abs(a), std::abs(b)), epsilon);
}

inline float Sign(float value)
{
    return (value > 0.0f) ? 1.0f : ((value < 0.0f) ? -1.0f : 0.0f);
}

inline float SignNoZero(float value)
{
    return value >= 0.0f ? 1.0f : -1.0f;
}

template<typename T>
inline T MaxZero(T value)
{
    return std::max(T(0), value);
}

// ==================== 范围映射 ====================

inline float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
{
    float denom = fromMax - fromMin;
    if (std::abs(denom) < EPSILON)
        return toMin;
    float t = (value - fromMin) / denom;
    return toMin + t * (toMax - toMin);
}

inline float Clamp01(float value)
{
    return Clamp(value, 0.0f, 1.0f);
}

} // namespace TE::Math
