// ToyEngine Core Module
// 随机数生成器

#pragma once

#include "MathTypes.h"
#include <cstdint>

namespace TE {

/// <summary>
/// 随机数生成器 - PCG 算法的简单实现
/// 提供高质量的伪随机数，用于游戏逻辑、粒子效果等
/// </summary>
class Random
{
public:
    // ==================== 种子设置 ====================

    /// <summary>
    /// 用指定种子初始化全局随机数生成器
    /// </summary>
    static void Seed(uint64_t seed);

    /// <summary>
    /// 用当前时间初始化全局随机数生成器
    /// </summary>
    static void SeedWithTime();

    // ==================== 浮点范围随机 ====================

    /// <summary>
    /// 随机浮点数 [min, max)
    /// </summary>
    [[nodiscard]] static float Range(float min, float max);

    /// <summary>
    /// 随机浮点数 [0, max)
    /// </summary>
    [[nodiscard]] static float Range(float max);

    /// <summary>
    /// 随机浮点数 [0, 1)
    /// </summary>
    [[nodiscard]] static float Value();

    // ==================== 整数范围随机 ====================

    /// <summary>
    /// 随机整数 [min, max]（包含边界）
    /// </summary>
    [[nodiscard]] static int Range(int min, int max);

    /// <summary>
    /// 随机整数 [0, max]（包含边界）
    /// </summary>
    [[nodiscard]] static int Range(int max);

    // ==================== 向量随机 ====================

    /// <summary>
    /// 随机单位向量（单位球面上均匀分布）
    /// </summary>
    [[nodiscard]] static Vector3 UnitVector();

    /// <summary>
    /// 随机单位向量（单位圆上均匀分布）
    /// </summary>
    [[nodiscard]] static Vector2 UnitCircle();

    /// <summary>
    /// 随机方向（仅 XZ 平面，Y=0）
    /// </summary>
    [[nodiscard]] static Vector3 DirectionXZ();

    /// <summary>
    /// 随机向量在指定范围内
    /// </summary>
    [[nodiscard]] static Vector3 Range(const Vector3& min, const Vector3& max);

    /// <summary>
    /// 随机向量在立方体内 [-extent, extent]
    /// </summary>
    [[nodiscard]] static Vector3 InsideCube(float extent);

    /// <summary>
    /// 随机向量在球体内
    /// </summary>
    [[nodiscard]] static Vector3 InsideSphere(float radius);

    // ==================== 特殊分布 ====================

    /// <summary>
    /// 高斯分布（正态分布）
    /// </summary>
    [[nodiscard]] static float Gaussian(float mean, float stdDev);

    /// <summary>
    /// 三角形分布
    /// </summary>
    [[nodiscard]] static float Triangle(float min, float max, float mode);

    /// <summary>
    /// 指数分布
    /// </summary>
    [[nodiscard]] static float Exponential(float lambda);

    // ==================== 概率与选择 ====================

    /// <summary>
    /// 概率测试 [0, 1]，probability 为真概率
    /// 例如 Bool(0.8f) 有 80% 概率返回 true
    /// </summary>
    [[nodiscard]] static bool Bool(float probability);

    /// <summary>
    /// 50/50 随机，等效于 Bool(0.5f)
    /// </summary>
    [[nodiscard]] static bool Bool();

    /// <summary>
    /// 符号随机（返回 -1 或 1）
    /// </summary>
    [[nodiscard]] static float Sign();

    /// <summary>
    /// 从数组中随机选择一个索引
    /// </summary>
    template<typename T>
    [[nodiscard]] static T& Select(T* array, int count)
    {
        return array[Range(0, count - 1)];
    }

    /// <summary>
    /// 打乱数组（Fisher-Yates 算法）
    /// </summary>
    template<typename T>
    static void Shuffle(T* array, int count)
    {
        for (int i = count - 1; i > 0; --i)
        {
            int j = Range(0, i);
            // 交换
            T temp = array[i];
            array[i] = array[j];
            array[j] = temp;
        }
    }

    // ==================== 实例化生成器 ====================

    /// <summary>
    /// 创建独立的随机数生成器实例
    /// 可用于需要独立随机序列的场景（如多线程）
    /// </summary>
    [[nodiscard]] static Random Create(uint64_t seed);

    /// <summary>
    /// 默认构造函数 - 使用当前时间作为种子
    /// </summary>
    Random();

    /// <summary>
    /// 指定种子构造函数
    /// </summary>
    explicit Random(uint64_t seed);

    /// <summary>
    /// 重新设置种子
    /// </summary>
    void SetSeed(uint64_t seed);

    // 实例方法（与静态方法相同功能）
    [[nodiscard]] float NextFloat();
    [[nodiscard]] float NextFloat(float min, float max);
    [[nodiscard]] int NextInt(int min, int max);
    [[nodiscard]] Vector3 NextUnitVector();

private:
    // PCG 随机数生成器状态
    uint64_t m_State;
    uint64_t m_Inc;

    // PCG 核心算法
    [[nodiscard]] uint32_t PCG32();

    // 生成 0~1 的浮点数
    [[nodiscard]] float PCGFloat();
};

} // namespace TE
