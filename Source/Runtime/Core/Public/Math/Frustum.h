// ToyEngine Core Module
// 视锥体 - 用于渲染管线的视锥剔除

#pragma once

#include "Geometry.h"
#include "MathUtils.h"

namespace TE {

/// <summary>
/// 视锥体平面索引
/// </summary>
enum class EFrustumPlane
{
    Left = 0,
    Right,
    Bottom,
    Top,
    Near,
    Far,
    Count
};

/// <summary>
/// 视锥体 - 由 6 个平面组成，用于场景剔除
/// 通常从相机的 View-Projection 矩阵提取
/// </summary>
struct Frustum
{
    /// <summary>
    /// 6 个裁剪平面：Left, Right, Bottom, Top, Near, Far
    /// </summary>
    Plane Planes[6];

    Frustum() = default;

    /// <summary>
    /// 从 View-Projection 矩阵提取视锥体（Gribb-Hartmann 方法）
    /// 提取的平面法线指向视锥体内部
    /// </summary>
    /// <param name="viewProj">视图投影矩阵（View * Projection）</param>
    [[nodiscard]] static Frustum FromViewProjection(const Matrix4& viewProj);

    /// <summary>
    /// 测试点是否在视锥体内
    /// </summary>
    [[nodiscard]] bool ContainsPoint(const Vector3& point) const;

    /// <summary>
    /// 测试轴对齐包围盒是否与视锥体相交
    /// 使用 P-vertex / N-vertex 优化，O(6) 复杂度
    /// </summary>
    [[nodiscard]] bool IntersectsAABB(const BoundingBox& box) const;

    /// <summary>
    /// 测试包围球是否与视锥体相交
    /// </summary>
    [[nodiscard]] bool IntersectsSphere(const BoundingSphere& sphere) const;

    /// <summary>
    /// 获取指定索引的平面
    /// </summary>
    [[nodiscard]] const Plane& GetPlane(EFrustumPlane plane) const
    {
        return Planes[static_cast<int>(plane)];
    }
};

} // namespace TE
