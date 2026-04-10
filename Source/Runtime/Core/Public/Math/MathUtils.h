// ToyEngine Core Module
// 向量扩展数学工具函数

#pragma once

#include "ScalarMath.h"
#include "MathTypes.h"

namespace TE::Math {
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

} // namespace TE::Math
