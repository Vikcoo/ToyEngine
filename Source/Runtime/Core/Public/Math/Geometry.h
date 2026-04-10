// ToyEngine Core Module
// 基础几何类型 - 射线、平面、包围盒、包围球

#pragma once

#include "MathTypes.h"
#include "ScalarMath.h"

namespace TE {

// ==================== 射线 ====================

/// <summary>
/// 射线 - 用于拾取、碰撞检测等
/// </summary>
struct Ray
{
    Vector3 Origin;      // 起点
    Vector3 Direction; // 方向（已归一化）

    Ray() : Origin(Vector3::Zero), Direction(Vector3::Forward) {}
    Ray(const Vector3& origin, const Vector3& direction)
        : Origin(origin)
        , Direction(direction.Normalize())
    {}

    /// <summary>
    /// 获取射线上某点的位置
    /// </summary>
    [[nodiscard]] Vector3 GetPoint(float distance) const
    {
        return Origin + Direction * distance;
    }
};

// ==================== 平面 ====================

/// <summary>
/// 平面 - 由法线和到原点的有符号距离定义
/// 方程：Ax + By + Cz + D = 0，其中 (A,B,C) 是法线，D 是距离
/// </summary>
struct Plane
{
    Vector3 Normal;   // 法线（应归一化）
    float Distance;   // 沿法线方向到原点的有符号距离

    Plane() : Normal(Vector3::Up), Distance(0.0f) {}
    Plane(const Vector3& normal, float distance)
        : Normal(normal.Normalize())
        , Distance(distance)
    {}
    Plane(const Vector3& normal, const Vector3& point)
        : Normal(normal.Normalize())
        , Distance(-Vector3::Dot(normal.Normalize(), point))
    {}

    /// <summary>
    /// 从三点创建平面
    /// </summary>
    [[nodiscard]] static Plane FromPoints(const Vector3& a, const Vector3& b, const Vector3& c)
    {
        Vector3 normal = Vector3::Cross(b - a, c - a).Normalize();
        return {normal, a};
    }

    /// <summary>
    /// 点是否在平面的正面
    /// </summary>
    [[nodiscard]] bool IsInFront(const Vector3& point) const
    {
        return SignedDistance(point) > 0.0f;
    }

    /// <summary>
    /// 点是否在平面的背面
    /// </summary>
    [[nodiscard]] bool IsBehind(const Vector3& point) const
    {
        return SignedDistance(point) < 0.0f;
    }

    /// <summary>
    /// 点是否在平面上（考虑浮点误差）
    /// </summary>
    [[nodiscard]] bool IsOnPlane(const Vector3& point, float epsilon = 1e-5f) const
    {
        return std::abs(SignedDistance(point)) < epsilon;
    }

    /// <summary>
    /// 获取点到平面的有符号距离
    /// </summary>
    [[nodiscard]] float SignedDistance(const Vector3& point) const
    {
        return Vector3::Dot(Normal, point) + Distance;
    }

    /// <summary>
    /// 获取点到平面的绝对距离
    /// </summary>
    [[nodiscard]] float DistanceToPoint(const Vector3& point) const
    {
        return std::abs(SignedDistance(point));
    }

    /// <summary>
    /// 获取点在平面上的最近点
    /// </summary>
    [[nodiscard]] Vector3 ClosestPoint(const Vector3& point) const
    {
        return point - Normal * SignedDistance(point);
    }

    /// <summary>
    /// 射线与平面相交检测
    /// </summary>
    /// <param name="ray">射线</param>
    /// <param name="distance">输出交点距离（沿射线）</param>
    /// <returns>是否相交</returns>
    [[nodiscard]] bool IntersectRay(const Ray& ray, float& distance) const
    {
        float denom = Vector3::Dot(Normal, ray.Direction);
        if (std::abs(denom) < 1e-6f)
        {
            // 射线与平面平行
            return false;
        }

        distance = -(Vector3::Dot(Normal, ray.Origin) + Distance) / denom;
        return distance >= 0.0f;
    }

    /// <summary>
    /// 射线与平面相交检测（包含交点位置）
    /// </summary>
    [[nodiscard]] bool IntersectRay(const Ray& ray, float& distance, Vector3& point) const
    {
        if (IntersectRay(ray, distance))
        {
            point = ray.GetPoint(distance);
            return true;
        }
        return false;
    }

    /// <summary>
    /// 翻转平面方向
    /// </summary>
    [[nodiscard]] Plane Flipped() const
    {
        return {-Normal, -Distance};
    }

    /// <summary>
    /// 归一化平面（确保法线长度为 1）
    /// </summary>
    void Normalize()
    {
        float len = Normal.Length();
        if (len > 1e-6f)
        {
            Normal = Normal / len;
            Distance = Distance / len;
        }
    }
};

// ==================== 轴对齐包围盒 (AABB) ====================

/// <summary>
/// 轴对齐包围盒 - 与坐标轴对齐的包围盒
/// </summary>
struct BoundingBox
{
    Vector3 Min;
    Vector3 Max;

    BoundingBox()
        : Min(Vector3::Zero)
        , Max(Vector3::Zero)
    {}

    BoundingBox(const Vector3& min, const Vector3& max)
        : Min(min)
        , Max(max)
    {}

    /// <summary>
    /// 从中心和大小创建
    /// </summary>
    [[nodiscard]] static BoundingBox FromCenterExtents(const Vector3& center, const Vector3& extents)
    {
        return {center - extents, center + extents};
    }

    /// <summary>
    /// 从中心和半宽创建（uniform）
    /// </summary>
    [[nodiscard]] static BoundingBox FromCenterHalfSize(const Vector3& center, float halfSize)
    {
        Vector3 ext(halfSize, halfSize, halfSize);
        return {center - ext, center + ext};
    }

    /// <summary>
    /// 获取中心点
    /// </summary>
    [[nodiscard]] Vector3 GetCenter() const
    {
        return (Min + Max) * 0.5f;
    }

    /// <summary>
    /// 获取半边长（extents）
    /// </summary>
    [[nodiscard]] Vector3 GetExtents() const
    {
        return (Max - Min) * 0.5f;
    }

    /// <summary>
    /// 获取大小
    /// </summary>
    [[nodiscard]] Vector3 GetSize() const
    {
        return Max - Min;
    }

    /// <summary>
    /// 获取对角线长度
    /// </summary>
    [[nodiscard]] float GetDiagonal() const
    {
        return GetSize().Length();
    }

    /// <summary>
    /// 获取表面积
    /// </summary>
    [[nodiscard]] float GetSurfaceArea() const
    {
        Vector3 size = GetSize();
        return 2.0f * (size.X * size.Y + size.X * size.Z + size.Y * size.Z);
    }

    /// <summary>
    /// 获取体积
    /// </summary>
    [[nodiscard]] float GetVolume() const
    {
        Vector3 size = GetSize();
        return size.X * size.Y * size.Z;
    }

    /// <summary>
    /// 是否包含点
    /// </summary>
    [[nodiscard]] bool Contains(const Vector3& point) const
    {
        return point.X >= Min.X && point.X <= Max.X &&
               point.Y >= Min.Y && point.Y <= Max.Y &&
               point.Z >= Min.Z && point.Z <= Max.Z;
    }

    /// <summary>
    /// 是否完全包含另一个包围盒
    /// </summary>
    [[nodiscard]] bool Contains(const BoundingBox& other) const
    {
        return Contains(other.Min) && Contains(other.Max);
    }

    /// <summary>
    /// 是否与另一个包围盒相交
    /// </summary>
    [[nodiscard]] bool Intersects(const BoundingBox& other) const
    {
        return Min.X <= other.Max.X && Max.X >= other.Min.X &&
               Min.Y <= other.Max.Y && Max.Y >= other.Min.Y &&
               Min.Z <= other.Max.Z && Max.Z >= other.Min.Z;
    }

    /// <summary>
    /// 射线相交检测
    /// </summary>
    [[nodiscard]] bool IntersectRay(const Ray& ray, float& distance) const
    {
        float tmin = 0.0f;
        float tmax = 1e30f;

        // X 轴
        if (std::abs(ray.Direction.X) < 1e-6f)
        {
            if (ray.Origin.X < Min.X || ray.Origin.X > Max.X)
                return false;
        }
        else
        {
            float tx1 = (Min.X - ray.Origin.X) / ray.Direction.X;
            float tx2 = (Max.X - ray.Origin.X) / ray.Direction.X;
            tmin = std::max(tmin, std::min(tx1, tx2));
            tmax = std::min(tmax, std::max(tx1, tx2));
        }

        // Y 轴
        if (std::abs(ray.Direction.Y) < 1e-6f)
        {
            if (ray.Origin.Y < Min.Y || ray.Origin.Y > Max.Y)
                return false;
        }
        else
        {
            float ty1 = (Min.Y - ray.Origin.Y) / ray.Direction.Y;
            float ty2 = (Max.Y - ray.Origin.Y) / ray.Direction.Y;
            tmin = std::max(tmin, std::min(ty1, ty2));
            tmax = std::min(tmax, std::max(ty1, ty2));
        }

        // Z 轴
        if (std::abs(ray.Direction.Z) < 1e-6f)
        {
            if (ray.Origin.Z < Min.Z || ray.Origin.Z > Max.Z)
                return false;
        }
        else
        {
            float tz1 = (Min.Z - ray.Origin.Z) / ray.Direction.Z;
            float tz2 = (Max.Z - ray.Origin.Z) / ray.Direction.Z;
            tmin = std::max(tmin, std::min(tz1, tz2));
            tmax = std::min(tmax, std::max(tz1, tz2));
        }

        distance = tmin;
        return tmax >= tmin && tmax >= 0.0f;
    }

    /// <summary>
    /// 扩展到包含点
    /// </summary>
    void Expand(const Vector3& point)
    {
        Min.X = std::min(Min.X, point.X);
        Min.Y = std::min(Min.Y, point.Y);
        Min.Z = std::min(Min.Z, point.Z);
        Max.X = std::max(Max.X, point.X);
        Max.Y = std::max(Max.Y, point.Y);
        Max.Z = std::max(Max.Z, point.Z);
    }

    /// <summary>
    /// 扩展到包含另一个包围盒
    /// </summary>
    void Expand(const BoundingBox& other)
    {
        Expand(other.Min);
        Expand(other.Max);
    }

    /// <summary>
    /// 膨胀/收缩包围盒
    /// </summary>
    void Inflate(float amount)
    {
        Vector3 delta(amount, amount, amount);
        Min -= delta;
        Max += delta;
    }

    /// <summary>
    /// 获取八个角点
    /// </summary>
    void GetCorners(Vector3 corners[8]) const
    {
        corners[0] = Vector3(Min.X, Min.Y, Min.Z);
        corners[1] = Vector3(Max.X, Min.Y, Min.Z);
        corners[2] = Vector3(Min.X, Max.Y, Min.Z);
        corners[3] = Vector3(Max.X, Max.Y, Min.Z);
        corners[4] = Vector3(Min.X, Min.Y, Max.Z);
        corners[5] = Vector3(Max.X, Min.Y, Max.Z);
        corners[6] = Vector3(Min.X, Max.Y, Max.Z);
        corners[7] = Vector3(Max.X, Max.Y, Max.Z);
    }

    /// <summary>
    /// 获取最近点（在包围盒内的点或表面上的点）
    /// </summary>
    [[nodiscard]] Vector3 ClosestPoint(const Vector3& point) const
    {
        return {
            std::max(Min.X, std::min(point.X, Max.X)),
            std::max(Min.Y, std::min(point.Y, Max.Y)),
            std::max(Min.Z, std::min(point.Z, Max.Z))
        };
    }

    /// <summary>
    /// 到点的平方距离
    /// </summary>
    [[nodiscard]] float DistanceSquared(const Vector3& point) const
    {
        Vector3 closest = ClosestPoint(point);
        return Vector3::DistanceSquared(point, closest);
    }

    /// <summary>
    /// 到点的距离
    /// </summary>
    [[nodiscard]] float Distance(const Vector3& point) const
    {
        return std::sqrt(DistanceSquared(point));
    }
};

// ==================== 包围球 ====================

/// <summary>
/// 包围球 - 由中心和半径定义
/// </summary>
struct BoundingSphere
{
    Vector3 Center;
    float Radius;

    BoundingSphere()
        : Center(Vector3::Zero)
        , Radius(0.0f)
    {}

    BoundingSphere(const Vector3& center, float radius)
        : Center(center)
        , Radius(radius)
    {}

    /// <summary>
    /// 从包围盒创建最小包围球
    /// </summary>
    [[nodiscard]] static BoundingSphere FromBox(const BoundingBox& box)
    {
        return {box.GetCenter(), box.GetExtents().Length()};
    }

    /// <summary>
    /// 是否包含点
    /// </summary>
    [[nodiscard]] bool Contains(const Vector3& point) const
    {
        return Vector3::DistanceSquared(Center, point) <= Radius * Radius;
    }

    /// <summary>
    /// 是否完全包含另一个包围球
    /// </summary>
    [[nodiscard]] bool Contains(const BoundingSphere& other) const
    {
        float dist = Vector3::Distance(Center, other.Center);
        return dist + other.Radius <= Radius;
    }

    /// <summary>
    /// 是否与另一个包围球相交
    /// </summary>
    [[nodiscard]] bool Intersects(const BoundingSphere& other) const
    {
        float distSq = Vector3::DistanceSquared(Center, other.Center);
        float radSum = Radius + other.Radius;
        return distSq <= radSum * radSum;
    }

    /// <summary>
    /// 是否与包围盒相交
    /// </summary>
    [[nodiscard]] bool Intersects(const BoundingBox& box) const
    {
        Vector3 closest = box.ClosestPoint(Center);
        return Vector3::DistanceSquared(Center, closest) <= Radius * Radius;
    }

    /// <summary>
    /// 射线相交检测
    /// </summary>
    [[nodiscard]] bool IntersectRay(const Ray& ray, float& distance) const
    {
        Vector3 oc = ray.Origin - Center;
        float a = Vector3::Dot(ray.Direction, ray.Direction);
        float b = 2.0f * Vector3::Dot(oc, ray.Direction);
        float c = Vector3::Dot(oc, oc) - Radius * Radius;

        float discriminant = b * b - 4.0f * a * c;
        if (discriminant < 0.0f)
            return false;

        float sqrtDisc = std::sqrt(discriminant);
        float t1 = (-b - sqrtDisc) / (2.0f * a);
        float t2 = (-b + sqrtDisc) / (2.0f * a);

        if (t1 >= 0.0f)
        {
            distance = t1;
            return true;
        }
        if (t2 >= 0.0f)
        {
            distance = t2;
            return true;
        }
        return false;
    }

    /// <summary>
    /// 射线相交检测（包含交点）
    /// </summary>
    [[nodiscard]] bool IntersectRay(const Ray& ray, float& distance, Vector3& point) const
    {
        if (IntersectRay(ray, distance))
        {
            point = ray.GetPoint(distance);
            return true;
        }
        return false;
    }

    /// <summary>
    /// 膨胀
    /// </summary>
    void Inflate(float amount)
    {
        Radius += amount;
    }

    /// <summary>
    /// 扩展到包含点
    /// </summary>
    void Expand(const Vector3& point)
    {
        float dist = Vector3::Distance(Center, point);
        if (dist > Radius)
        {
            // 扩展球体以包含该点
            float newRadius = (Radius + dist) * 0.5f;
            Center = Center + (point - Center) * ((newRadius - Radius) / dist);
            Radius = newRadius;
        }
    }

    /// <summary>
    /// 扩展到包含另一个包围球
    /// </summary>
    void Expand(const BoundingSphere& other)
    {
        float dist = Vector3::Distance(Center, other.Center);
        if (dist + other.Radius > Radius)
        {
            // 新的中心在两个中心之间
            Vector3 newCenter = (Center * Radius + other.Center * other.Radius) / (Radius + other.Radius);
            Radius = (dist + Radius + other.Radius) * 0.5f;
            Center = newCenter;
        }
    }

    /// <summary>
    /// 到点的距离（点在球外为正，球内为负）
    /// </summary>
    [[nodiscard]] float Distance(const Vector3& point) const
    {
        return Vector3::Distance(Center, point) - Radius;
    }

    /// <summary>
    /// 获取包围盒
    /// </summary>
    [[nodiscard]] BoundingBox ToBox() const
    {
        Vector3 ext(Radius, Radius, Radius);
        return {Center - ext, Center + ext};
    }
};

// ==================== 几何工具函数 ====================

/// <summary>
/// 计算三角形面积
/// </summary>
[[nodiscard]] inline float TriangleArea(const Vector3& a, const Vector3& b, const Vector3& c)
{
    return Vector3::Cross(b - a, c - a).Length() * 0.5f;
}

/// <summary>
/// 计算三角形法线（归一化）
/// </summary>
[[nodiscard]] inline Vector3 TriangleNormal(const Vector3& a, const Vector3& b, const Vector3& c)
{
    return Vector3::Cross(b - a, c - a).Normalize();
}

/// <summary>
/// 点到线段的最近点
/// </summary>
[[nodiscard]] inline Vector3 ClosestPointOnSegment(const Vector3& point, const Vector3& a, const Vector3& b)
{
    Vector3 ab = b - a;
    float t = Vector3::Dot(point - a, ab) / Vector3::Dot(ab, ab);
    t = Math::Clamp(t, 0.0f, 1.0f);
    return a + ab * t;
}

/// <summary>
/// 点到线段的平方距离
/// </summary>
[[nodiscard]] inline float DistancePointToSegmentSquared(const Vector3& point, const Vector3& a, const Vector3& b)
{
    Vector3 closest = ClosestPointOnSegment(point, a, b);
    return Vector3::DistanceSquared(point, closest);
}

/// <summary>
/// 球体与球体合并为最小包围球
/// </summary>
[[nodiscard]] inline BoundingSphere MergeSpheres(const BoundingSphere& a, const BoundingSphere& b)
{
    BoundingSphere result = a;
    result.Expand(b);
    return result;
}

/// <summary>
/// 包围盒与包围盒合并
/// </summary>
[[nodiscard]] inline BoundingBox MergeBoxes(const BoundingBox& a, const BoundingBox& b)
{
    BoundingBox result = a;
    result.Expand(b);
    return result;
}

} // namespace TE
