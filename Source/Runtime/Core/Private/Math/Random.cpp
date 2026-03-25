// ToyEngine Core Module
// 随机数生成器实现 - PCG (Permuted Congruential Generator)

#include "Math/Random.h"
#include "Math/MathUtils.h"
#include <chrono>
#include <cmath>

namespace TE {

// ==================== 全局随机数生成器 ====================

namespace {
    // 全局状态
    uint64_t g_State = 0x853c49e6748fea9bULL;
    uint64_t g_Inc = 0xda3e39cb94b95bdbULL;

    // PCG32 核心算法 - 重命名避免与 Random 类方法冲突
    uint32_t PCG32Impl(uint64_t& state, uint64_t& inc)
    {
        uint64_t oldState = state;
        state = oldState * 6364136223846793005ULL + (inc | 1);
        uint32_t xorShifted = static_cast<uint32_t>(((oldState >> 18u) ^ oldState) >> 27u);
        uint32_t rot = static_cast<uint32_t>(oldState >> 59u);
        return (xorShifted >> rot) | (xorShifted << ((-static_cast<int32_t>(rot)) & 31));
    }

    // 生成 0~1 浮点数 - 重命名避免与 Random 类方法冲突
    float PCGFloatImpl(uint64_t& state, uint64_t& inc)
    {
        // 生成 24 位精度的浮点数
        uint32_t bits = PCG32Impl(state, inc);
        return (bits >> 8) / 16777216.0f;
    }
} // anonymous namespace

// ==================== 静态方法实现 ====================

void Random::Seed(uint64_t seed)
{
    g_State = 0U;
    g_Inc = (seed << 1u) | 1u;
    PCG32Impl(g_State, g_Inc);
    g_State += seed;
    PCG32Impl(g_State, g_Inc);
}

void Random::SeedWithTime()
{
    auto now = std::chrono::high_resolution_clock::now();
    auto nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    Seed(static_cast<uint64_t>(nanos));
}

float Random::Range(float min, float max)
{
    return min + PCGFloatImpl(g_State, g_Inc) * (max - min);
}

float Random::Range(float max)
{
    return PCGFloatImpl(g_State, g_Inc) * max;
}

float Random::Value()
{
    return PCGFloatImpl(g_State, g_Inc);
}

int Random::Range(int min, int max)
{
    if (min >= max) return min;

    // 有偏消除：拒绝采样
    uint32_t range = static_cast<uint32_t>(max - min + 1);
    uint32_t threshold = (0xFFFFFFFFu / range) * range;

    uint32_t value;
    do {
        value = PCG32Impl(g_State, g_Inc);
    } while (value >= threshold);

    return min + static_cast<int>(value % range);
}

int Random::Range(int max)
{
    return Range(0, max);
}

Vector3 Random::UnitVector()
{
    // Marsaglia 方法：在单位立方体内随机选择，拒绝圆外点
    float x, y, z, d2;
    do {
        x = Range(-1.0f, 1.0f);
        y = Range(-1.0f, 1.0f);
        z = Range(-1.0f, 1.0f);
        d2 = x * x + y * y + z * z;
    } while (d2 > 1.0f || d2 < 0.0001f);

    float scale = 1.0f / Math::Sqrt(d2);
    return {x * scale, y * scale, z * scale};
}

Vector2 Random::UnitCircle()
{
    // 类似 Marsaglia 方法
    float x, y, d2;
    do {
        x = Range(-1.0f, 1.0f);
        y = Range(-1.0f, 1.0f);
        d2 = x * x + y * y;
    } while (d2 > 1.0f || d2 < 0.0001f);

    float scale = 1.0f / Math::Sqrt(d2);
    return {x * scale, y * scale};
}

Vector3 Random::DirectionXZ()
{
    Vector2 circle = UnitCircle();
    return {circle.X, 0.0f, circle.Y};
}

Vector3 Random::Range(const Vector3& min, const Vector3& max)
{
    return {
        Range(min.X, max.X),
        Range(min.Y, max.Y),
        Range(min.Z, max.Z)
    };
}

Vector3 Random::InsideCube(float extent)
{
    return {
        Range(-extent, extent),
        Range(-extent, extent),
        Range(-extent, extent)
    };
}

Vector3 Random::InsideSphere(float radius)
{
    // 在单位球内均匀分布
    float u = Range(0.0f, 1.0f);
    float v = Range(0.0f, 1.0f);
    float theta = 2.0f * Math::PI * u;
    float phi = Math::Acos(2.0f * v - 1.0f);
    float r = Math::Pow(Range(0.0f, 1.0f), 1.0f / 3.0f) * radius;

    float sinPhi = Math::Sin(phi);
    return {
        r * sinPhi * Math::Cos(theta),
        r * sinPhi * Math::Sin(theta),
        r * Math::Cos(phi)
    };
}

float Random::Gaussian(float mean, float stdDev)
{
    // Box-Muller 变换
    static bool hasSpare = false;
    static float spare;

    if (hasSpare)
    {
        hasSpare = false;
        return mean + stdDev * spare;
    }

    float u, v, s;
    do {
        u = Range(-1.0f, 1.0f);
        v = Range(-1.0f, 1.0f);
        s = u * u + v * v;
    } while (s >= 1.0f || s < 0.0001f);

    float mul = Math::Sqrt(-2.0f * Math::Log(s) / s);
    spare = v * mul;
    hasSpare = true;

    return mean + stdDev * u * mul;
}

float Random::Triangle(float min, float max, float mode)
{
    float u = Range(0.0f, 1.0f);
    float c = (mode - min) / (max - min);

    if (u <= c)
    {
        return min + Math::Sqrt(u * (max - min) * (mode - min));
    }
    else
    {
        return max - Math::Sqrt((1.0f - u) * (max - min) * (max - mode));
    }
}

float Random::Exponential(float lambda)
{
    return -Math::Log(1.0f - Value()) / lambda;
}

bool Random::Bool(float probability)
{
    return PCGFloatImpl(g_State, g_Inc) < probability;
}

bool Random::Bool()
{
    return (PCG32Impl(g_State, g_Inc) & 1) != 0;
}

float Random::Sign()
{
    // 显式调用无参数的 Bool() 避免歧义
    return Random::Bool() ? 1.0f : -1.0f;
}

// ==================== 实例化生成器 ====================

Random Random::Create(uint64_t seed)
{
    Random rng;
    rng.SetSeed(seed);
    return rng;
}

Random::Random()
{
    SeedWithTime();
    m_State = g_State;
    m_Inc = g_Inc;
}

Random::Random(uint64_t seed)
{
    SetSeed(seed);
}

void Random::SetSeed(uint64_t seed)
{
    m_State = 0U;
    m_Inc = (seed << 1u) | 1u;
    PCG32Impl(m_State, m_Inc);
    m_State += seed;
    PCG32Impl(m_State, m_Inc);
}

uint32_t Random::PCG32()
{
    return PCG32Impl(m_State, m_Inc);
}

float Random::PCGFloat()
{
    return PCGFloatImpl(m_State, m_Inc);
}

float Random::NextFloat()
{
    return PCGFloat();
}

float Random::NextFloat(float min, float max)
{
    return min + PCGFloat() * (max - min);
}

int Random::NextInt(int min, int max)
{
    if (min >= max) return min;

    uint32_t range = static_cast<uint32_t>(max - min + 1);
    uint32_t threshold = (0xFFFFFFFFu / range) * range;

    uint32_t value;
    do {
        value = PCG32();
    } while (value >= threshold);

    return min + static_cast<int>(value % range);
}

Vector3 Random::NextUnitVector()
{
    float x, y, z, d2;
    do {
        x = NextFloat(-1.0f, 1.0f);
        y = NextFloat(-1.0f, 1.0f);
        z = NextFloat(-1.0f, 1.0f);
        d2 = x * x + y * y + z * z;
    } while (d2 > 1.0f || d2 < 0.0001f);

    float scale = 1.0f / Math::Sqrt(d2);
    return {x * scale, y * scale, z * scale};
}

} // namespace TE
