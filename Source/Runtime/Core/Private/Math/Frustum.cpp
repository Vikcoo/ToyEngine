// ToyEngine Core Module
// 视锥体实现

#include "Math/Frustum.h"
#include <cmath>

namespace TE {

// Gribb-Hartmann 方法：从右手系、ZO 深度范围的 VP 矩阵行提取 6 个裁剪平面。
// 矩阵按列主序存储：M[col][row]
// 行向量通过 row 索引取各列元素
Frustum Frustum::FromViewProjectionRH_ZO(const Matrix4& vp)
{
    Frustum frustum;

    const auto makePlane = [](float a, float b, float c, float d)
    {
        return Plane(Vector3(a, b, c), d);
    };

    // 提取矩阵各行（row 为行索引，col 为列索引，vp.M[col][row]）
    // row0 = (M[0][0], M[1][0], M[2][0], M[3][0])
    // row1 = (M[0][1], M[1][1], M[2][1], M[3][1])
    // row2 = (M[0][2], M[1][2], M[2][2], M[3][2])
    // row3 = (M[0][3], M[1][3], M[2][3], M[3][3])

    // Left:   row3 + row0
    {
        const float a = vp.M[0][3] + vp.M[0][0];
        const float b = vp.M[1][3] + vp.M[1][0];
        const float c = vp.M[2][3] + vp.M[2][0];
        const float d = vp.M[3][3] + vp.M[3][0];
        frustum.Planes[0] = makePlane(a, b, c, d);
    }

    // Right:  row3 - row0
    {
        const float a = vp.M[0][3] - vp.M[0][0];
        const float b = vp.M[1][3] - vp.M[1][0];
        const float c = vp.M[2][3] - vp.M[2][0];
        const float d = vp.M[3][3] - vp.M[3][0];
        frustum.Planes[1] = makePlane(a, b, c, d);
    }

    // Bottom: row3 + row1
    {
        const float a = vp.M[0][3] + vp.M[0][1];
        const float b = vp.M[1][3] + vp.M[1][1];
        const float c = vp.M[2][3] + vp.M[2][1];
        const float d = vp.M[3][3] + vp.M[3][1];
        frustum.Planes[2] = makePlane(a, b, c, d);
    }

    // Top:    row3 - row1
    {
        const float a = vp.M[0][3] - vp.M[0][1];
        const float b = vp.M[1][3] - vp.M[1][1];
        const float c = vp.M[2][3] - vp.M[2][1];
        const float d = vp.M[3][3] - vp.M[3][1];
        frustum.Planes[3] = makePlane(a, b, c, d);
    }

    // Near:   row2。ZO 深度范围的裁剪条件是 z >= 0。
    {
        const float a = vp.M[0][2];
        const float b = vp.M[1][2];
        const float c = vp.M[2][2];
        const float d = vp.M[3][2];
        frustum.Planes[4] = makePlane(a, b, c, d);
    }

    // Far:    row3 - row2
    {
        const float a = vp.M[0][3] - vp.M[0][2];
        const float b = vp.M[1][3] - vp.M[1][2];
        const float c = vp.M[2][3] - vp.M[2][2];
        const float d = vp.M[3][3] - vp.M[3][2];
        frustum.Planes[5] = makePlane(a, b, c, d);
    }

    return frustum;
}

bool Frustum::ContainsPoint(const Vector3& point) const
{
    for (int i = 0; i < 6; ++i)
    {
        if (Planes[i].SignedDistance(point) < 0.0f)
            return false;
    }
    return true;
}

// AABB 相交测试：P-vertex / N-vertex 优化
// 对于每个平面，只需要测试 AABB 的"最远正顶点"（P-vertex）
// 如果 P-vertex 在平面负半空间，则 AABB 完全在外
bool Frustum::IntersectsAABB(const BoundingBox& box) const
{
    for (int i = 0; i < 6; ++i)
    {
        const Plane& plane = Planes[i];

        // 计算 P-vertex（沿法线方向最远的顶点）
        Vector3 pVertex;
        pVertex.X = (plane.Normal.X >= 0.0f) ? box.Max.X : box.Min.X;
        pVertex.Y = (plane.Normal.Y >= 0.0f) ? box.Max.Y : box.Min.Y;
        pVertex.Z = (plane.Normal.Z >= 0.0f) ? box.Max.Z : box.Min.Z;

        // 如果 P-vertex 在平面负半空间，AABB 完全在视锥体外
        if (plane.SignedDistance(pVertex) < 0.0f)
            return false;
    }
    return true;
}

bool Frustum::IntersectsSphere(const BoundingSphere& sphere) const
{
    for (int i = 0; i < 6; ++i)
    {
        // 球心到平面的有符号距离 < -radius 意味着球完全在平面外
        if (Planes[i].SignedDistance(sphere.Center) < -sphere.Radius)
            return false;
    }
    return true;
}

} // namespace TE
