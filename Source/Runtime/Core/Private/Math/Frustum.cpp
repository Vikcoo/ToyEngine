// ToyEngine Core Module
// 视锥体实现

#include "Math/Frustum.h"
#include <cmath>

namespace TE {

// Gribb-Hartmann 方法：从 VP 矩阵行提取 6 个裁剪平面
// 矩阵按列主序存储：M[col][row]
// 行向量通过 row 索引取各列元素
Frustum Frustum::FromViewProjection(const Matrix4& vp)
{
    Frustum frustum;

    // 提取矩阵各行（row 为行索引，col 为列索引，vp.M[col][row]）
    // row0 = (M[0][0], M[1][0], M[2][0], M[3][0])
    // row1 = (M[0][1], M[1][1], M[2][1], M[3][1])
    // row2 = (M[0][2], M[1][2], M[2][2], M[3][2])
    // row3 = (M[0][3], M[1][3], M[2][3], M[3][3])

    // Left:   row3 + row0
    {
        float a = vp.M[0][3] + vp.M[0][0];
        float b = vp.M[1][3] + vp.M[1][0];
        float c = vp.M[2][3] + vp.M[2][0];
        float d = vp.M[3][3] + vp.M[3][0];
        float len = std::sqrt(a * a + b * b + c * c);
        if (len > 1e-6f)
        {
            frustum.Planes[0] = Plane(Vector3(a, b, c), d);
            frustum.Planes[0].Normal = Vector3(a / len, b / len, c / len);
            frustum.Planes[0].Distance = d / len;
        }
    }

    // Right:  row3 - row0
    {
        float a = vp.M[0][3] - vp.M[0][0];
        float b = vp.M[1][3] - vp.M[1][0];
        float c = vp.M[2][3] - vp.M[2][0];
        float d = vp.M[3][3] - vp.M[3][0];
        float len = std::sqrt(a * a + b * b + c * c);
        if (len > 1e-6f)
        {
            frustum.Planes[1] = Plane(Vector3(a, b, c), d);
            frustum.Planes[1].Normal = Vector3(a / len, b / len, c / len);
            frustum.Planes[1].Distance = d / len;
        }
    }

    // Bottom: row3 + row1
    {
        float a = vp.M[0][3] + vp.M[0][1];
        float b = vp.M[1][3] + vp.M[1][1];
        float c = vp.M[2][3] + vp.M[2][1];
        float d = vp.M[3][3] + vp.M[3][1];
        float len = std::sqrt(a * a + b * b + c * c);
        if (len > 1e-6f)
        {
            frustum.Planes[2] = Plane(Vector3(a, b, c), d);
            frustum.Planes[2].Normal = Vector3(a / len, b / len, c / len);
            frustum.Planes[2].Distance = d / len;
        }
    }

    // Top:    row3 - row1
    {
        float a = vp.M[0][3] - vp.M[0][1];
        float b = vp.M[1][3] - vp.M[1][1];
        float c = vp.M[2][3] - vp.M[2][1];
        float d = vp.M[3][3] - vp.M[3][1];
        float len = std::sqrt(a * a + b * b + c * c);
        if (len > 1e-6f)
        {
            frustum.Planes[3] = Plane(Vector3(a, b, c), d);
            frustum.Planes[3].Normal = Vector3(a / len, b / len, c / len);
            frustum.Planes[3].Distance = d / len;
        }
    }

    // Near:   row3 + row2
    {
        float a = vp.M[0][3] + vp.M[0][2];
        float b = vp.M[1][3] + vp.M[1][2];
        float c = vp.M[2][3] + vp.M[2][2];
        float d = vp.M[3][3] + vp.M[3][2];
        float len = std::sqrt(a * a + b * b + c * c);
        if (len > 1e-6f)
        {
            frustum.Planes[4] = Plane(Vector3(a, b, c), d);
            frustum.Planes[4].Normal = Vector3(a / len, b / len, c / len);
            frustum.Planes[4].Distance = d / len;
        }
    }

    // Far:    row3 - row2
    {
        float a = vp.M[0][3] - vp.M[0][2];
        float b = vp.M[1][3] - vp.M[1][2];
        float c = vp.M[2][3] - vp.M[2][2];
        float d = vp.M[3][3] - vp.M[3][2];
        float len = std::sqrt(a * a + b * b + c * c);
        if (len > 1e-6f)
        {
            frustum.Planes[5] = Plane(Vector3(a, b, c), d);
            frustum.Planes[5].Normal = Vector3(a / len, b / len, c / len);
            frustum.Planes[5].Distance = d / len;
        }
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
