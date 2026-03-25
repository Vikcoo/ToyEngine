// ToyEngine Core Module
// 颜色类实现

#include "Math/Color.h"
#include "Math/MathUtils.h"
#include <cmath>

namespace TE {

// ==================== 预设颜色常量 ====================
const Color Color::White(1.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Black(0.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Red(1.0f, 0.0f, 0.0f, 1.0f);
const Color Color::Green(0.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Blue(0.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Yellow(1.0f, 1.0f, 0.0f, 1.0f);
const Color Color::Cyan(0.0f, 1.0f, 1.0f, 1.0f);
const Color Color::Magenta(1.0f, 0.0f, 1.0f, 1.0f);
const Color Color::Gray(0.5f, 0.5f, 0.5f, 1.0f);
const Color Color::LightGray(0.75f, 0.75f, 0.75f, 1.0f);
const Color Color::DarkGray(0.25f, 0.25f, 0.25f, 1.0f);
const Color Color::Orange(1.0f, 0.5f, 0.0f, 1.0f);
const Color Color::Purple(0.5f, 0.0f, 0.5f, 1.0f);
const Color Color::Brown(0.6f, 0.3f, 0.1f, 1.0f);
const Color Color::Transparent(0.0f, 0.0f, 0.0f, 0.0f);

// ==================== HSV 转换 ====================

Color Color::FromHSV(float h, float s, float v, float a)
{
    // 归一化色相
    h = Math::Repeat(h, 360.0f);
    s = Math::Clamp(s, 0.0f, 1.0f);
    v = Math::Clamp(v, 0.0f, 1.0f);

    if (s <= 0.0f)
    {
        // 灰度
        return {v, v, v, a};
    }

    float hh = h / 60.0f;
    int i = static_cast<int>(hh);
    float ff = hh - i;

    float p = v * (1.0f - s);
    float q = v * (1.0f - s * ff);
    float t = v * (1.0f - s * (1.0f - ff));

    switch (i)
    {
    case 0: return {v, t, p, a};
    case 1: return {q, v, p, a};
    case 2: return {p, v, t, a};
    case 3: return {p, q, v, a};
    case 4: return {t, p, v, a};
    case 5:
    default: return {v, p, q, a};
    }
}

Vector3 Color::ToHSV() const
{
    float max = Math::Max(R, Math::Max(G, B));
    float min = Math::Min(R, Math::Min(G, B));
    float delta = max - min;

    float v = max;
    float s = (max == 0.0f) ? 0.0f : delta / max;
    float h = 0.0f;

    if (delta > 0.0f)
    {
        if (max == R)
        {
            h = 60.0f * (G - B) / delta;
            if (G < B) h += 360.0f;
        }
        else if (max == G)
        {
            h = 60.0f * (B - R) / delta + 120.0f;
        }
        else // max == B
        {
            h = 60.0f * (R - G) / delta + 240.0f;
        }
    }

    return {h, s, v};
}

// ==================== HSV 空间插值 ====================

Color Color::LerpHSV(const Color& a, const Color& b, float t)
{
    Vector3 hsvA = a.ToHSV();
    Vector3 hsvB = b.ToHSV();

    // 色相选择最短路径
    float hueDiff = hsvB.X - hsvA.X;
    if (hueDiff > 180.0f)
    {
        hsvA.X += 360.0f;
    }
    else if (hueDiff < -180.0f)
    {
        hsvB.X += 360.0f;
    }

    Vector3 result = Vector3::Lerp(hsvA, hsvB, t);
    result.X = Math::Repeat(result.X, 360.0f);

    return FromHSV(result.X, result.Y, result.Z, Math::Lerp(a.A, b.A, t));
}

} // namespace TE
