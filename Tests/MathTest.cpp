// ToyEngine - Math 模块完整测试
// 测试 MathTypes, Transform, MathUtils, Color, Random, Geometry

#include "Math/MathTypes.h"
#include "Math/Transform.h"
#include "Math/MathUtils.h"
#include "Math/Color.h"
#include "Math/Random.h"
#include "Math/Geometry.h"
#include "Math/Frustum.h"
#include "Math/VectorInt.h"
#include "Memory/Memory.h"

#include <iostream>
#include <cmath>
#include <algorithm>

namespace {

// 浮点比较辅助函数
bool ApproxEqual(float a, float b, float epsilon = 1e-5f)
{
    return std::abs(a - b) < epsilon;
}

bool ApproxEqual(const TE::Vector3& a, const TE::Vector3& b, float epsilon = 1e-5f)
{
    return ApproxEqual(a.X, b.X, epsilon) &&
           ApproxEqual(a.Y, b.Y, epsilon) &&
           ApproxEqual(a.Z, b.Z, epsilon);
}

// ==================== Vector 测试 ====================

bool TestVectorBasics()
{
    std::cout << "[MathTest] Vector basics...\n";

    // Vector2
    TE::Vector2 v2(3.0f, 4.0f);
    if (!ApproxEqual(v2.Length(), 5.0f)) {
        std::cerr << "[FAIL] Vector2 Length\n";
        return false;
    }

    TE::Vector2 v2norm = v2.Normalize();
    if (!ApproxEqual(v2norm.Length(), 1.0f)) {
        std::cerr << "[FAIL] Vector2 Normalize\n";
        return false;
    }

    // Vector3
    TE::Vector3 v3(1.0f, 2.0f, 2.0f);
    if (!ApproxEqual(v3.Length(), 3.0f)) {
        std::cerr << "[FAIL] Vector3 Length\n";
        return false;
    }

    // Vector3 Dot
    TE::Vector3 a(1.0f, 0.0f, 0.0f);
    TE::Vector3 b(0.0f, 1.0f, 0.0f);
    if (!ApproxEqual(TE::Vector3::Dot(a, b), 0.0f)) {
        std::cerr << "[FAIL] Vector3 Dot orthogonal\n";
        return false;
    }

    // Vector3 Cross
    TE::Vector3 cross = TE::Vector3::Cross(a, b);
    if (!ApproxEqual(cross, TE::Vector3(0.0f, 0.0f, 1.0f))) {
        std::cerr << "[FAIL] Vector3 Cross\n";
        return false;
    }

    // Vector4
    TE::Vector4 v4(1.0f, 2.0f, 3.0f, 4.0f);
    if (!ApproxEqual(v4.GetXYZ(), TE::Vector3(1.0f, 2.0f, 3.0f))) {
        std::cerr << "[FAIL] Vector4 GetXYZ\n";
        return false;
    }

    std::cout << "[MathTest] Vector basics passed.\n";
    return true;
}

// ==================== Matrix 测试 ====================

bool TestMatrixBasics()
{
    std::cout << "[MathTest] Matrix basics...\n";

    // Matrix4 Identity
    TE::Matrix4 identity;
    TE::Vector4 v(1.0f, 2.0f, 3.0f, 1.0f);
    TE::Vector4 result = identity * v;
    if (!ApproxEqual(result.X, 1.0f) || !ApproxEqual(result.Y, 2.0f) ||
        !ApproxEqual(result.Z, 3.0f) || !ApproxEqual(result.W, 1.0f)) {
        std::cerr << "[FAIL] Matrix4 Identity\n";
        return false;
    }

    // Matrix4 Translation
    TE::Matrix4 trans = TE::Matrix4::Translate(TE::Vector3(10.0f, 20.0f, 30.0f));
    TE::Vector4 vTrans = trans * TE::Vector4(0.0f, 0.0f, 0.0f, 1.0f);
    if (!ApproxEqual(vTrans.X, 10.0f) || !ApproxEqual(vTrans.Y, 20.0f) ||
        !ApproxEqual(vTrans.Z, 30.0f)) {
        std::cerr << "[FAIL] Matrix4 Translate\n";
        return false;
    }

    // Matrix4 Scale
    TE::Matrix4 scale = TE::Matrix4::Scale(TE::Vector3(2.0f, 3.0f, 4.0f));
    TE::Vector4 vScale = scale * TE::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    if (!ApproxEqual(vScale.X, 2.0f) || !ApproxEqual(vScale.Y, 3.0f) ||
        !ApproxEqual(vScale.Z, 4.0f)) {
        std::cerr << "[FAIL] Matrix4 Scale\n";
        return false;
    }

    // Matrix4 Multiply
    TE::Matrix4 combined = trans * scale;
    TE::Vector4 vCombo = combined * TE::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
    // 先缩放 (1,1,1) -> (2,3,4)，再平移 -> (12,23,34)
    if (!ApproxEqual(vCombo.X, 12.0f) || !ApproxEqual(vCombo.Y, 23.0f) ||
        !ApproxEqual(vCombo.Z, 34.0f)) {
        std::cerr << "[FAIL] Matrix4 Multiply order\n";
        return false;
    }

    // Matrix4 Inverse
    TE::Matrix4 inv = trans.Inverse();
    TE::Vector4 vBack = inv * vTrans;
    if (!ApproxEqual(vBack.X, 0.0f) || !ApproxEqual(vBack.Y, 0.0f) ||
        !ApproxEqual(vBack.Z, 0.0f)) {
        std::cerr << "[FAIL] Matrix4 Inverse\n";
        return false;
    }

    // LookAt
    TE::Matrix4 view = TE::Matrix4::LookAt(
        TE::Vector3(0.0f, 0.0f, 5.0f),  // eye
        TE::Vector3(0.0f, 0.0f, 0.0f),  // center
        TE::Vector3(0.0f, 1.0f, 0.0f)   // up
    );
    TE::Vector4 forward4 = view * TE::Vector4(0.0f, 0.0f, -1.0f, 0.0f);
    TE::Vector3 forward = forward4.GetXYZ();
    // 相机朝向 -Z，所以视图矩阵应该将 -Z 映射到世界空间的 Z 方向

    std::cout << "[MathTest] Matrix basics passed.\n";
    return true;
}

// ==================== Quaternion 测试 ====================

bool TestQuaternionBasics()
{
    std::cout << "[MathTest] Quaternion basics...\n";

    // Identity
    TE::Quat identity = TE::Quat::Identity;
    TE::Vector3 v(1.0f, 0.0f, 0.0f);
    TE::Vector3 rotated = identity * v;
    if (!ApproxEqual(rotated, v)) {
        std::cerr << "[FAIL] Quat Identity rotation\n";
        return false;
    }

    // 90 degree rotation around Y
    TE::Quat rotY(TE::Vector3::Up, TE::Math::PI / 2.0f);
    TE::Vector3 vX(1.0f, 0.0f, 0.0f);
    TE::Vector3 vRotated = rotY * vX;
    // Should be approximately (0, 0, -1)
    if (!ApproxEqual(vRotated.X, 0.0f, 1e-4f) ||
        !ApproxEqual(vRotated.Z, -1.0f, 1e-4f)) {
        std::cerr << "[FAIL] Quat 90 degree Y rotation\n";
        return false;
    }

    // Quat to Matrix and back
    TE::Matrix4 mat = rotY.ToMatrix4();
    TE::Vector4 v4Rot = mat * TE::Vector4(1.0f, 0.0f, 0.0f, 1.0f);
    if (!ApproxEqual(v4Rot.X, 0.0f, 1e-4f) ||
        !ApproxEqual(v4Rot.Z, -1.0f, 1e-4f)) {
        std::cerr << "[FAIL] Quat ToMatrix4\n";
        return false;
    }

    // Euler angles conversion
    TE::Quat fromEuler = TE::Quat::FromEuler(0.0f, TE::Math::PI / 2.0f, 0.0f); // pitch 90
    TE::Vector3 eulerBack = fromEuler.ToEulerAngles();
    if (!ApproxEqual(eulerBack.Y, TE::Math::PI / 2.0f, 1e-4f)) {
        std::cerr << "[FAIL] Quat Euler angles conversion\n";
        return false;
    }

    std::cout << "[MathTest] Quaternion basics passed.\n";
    return true;
}

// ==================== Transform 测试 ====================

bool TestTransformBasics()
{
    std::cout << "[MathTest] Transform basics...\n";

    // Basic transform
    TE::Transform t;
    t.Position = TE::Vector3(10.0f, 20.0f, 30.0f);
    t.Scale = TE::Vector3(2.0f, 2.0f, 2.0f);

    TE::Vector3 point(1.0f, 0.0f, 0.0f);
    TE::Vector3 transformed = t.TransformPoint(point);
    // (1,0,0) * 2 + (10,20,30) = (12, 20, 30)
    if (!ApproxEqual(transformed, TE::Vector3(12.0f, 20.0f, 30.0f))) {
        std::cerr << "[FAIL] Transform Point\n";
        return false;
    }

    // Transform ToMatrix
    TE::Matrix4 mat = t.ToMatrix();
    TE::Vector4 v4 = mat * TE::Vector4(1.0f, 0.0f, 0.0f, 1.0f);
    if (!ApproxEqual(v4.X, 12.0f) || !ApproxEqual(v4.Y, 20.0f) || !ApproxEqual(v4.Z, 30.0f)) {
        std::cerr << "[FAIL] Transform ToMatrix consistency\n";
        return false;
    }

    // LookAt
    TE::Transform lookAt = TE::Transform::LookAt(
        TE::Vector3(0.0f, 0.0f, 5.0f),
        TE::Vector3(0.0f, 0.0f, 0.0f),
        TE::Vector3::Up
    );
    TE::Vector3 forward = lookAt.GetForward();
    if (!ApproxEqual(forward.X, 0.0f, 1e-4f) ||
        !ApproxEqual(forward.Y, 0.0f, 1e-4f) ||
        !ApproxEqual(forward.Z, -1.0f, 1e-4f)) {
        std::cerr << "[FAIL] Transform LookAt GetForward\n";
        return false;
    }

    // Inverse transform
    TE::Transform inv = t.Inverse();
    TE::Vector3 back = inv.TransformPoint(transformed);
    if (!ApproxEqual(back, point)) {
        std::cerr << "[FAIL] Transform Inverse\n";
        return false;
    }

    // Euler angles
    TE::Transform euler = TE::Transform::FromEulerDegrees(90.0f, 0.0f, 0.0f);
    TE::Vector3 eulerDeg = euler.GetEulerAnglesDegrees();
    if (!ApproxEqual(eulerDeg.X, 90.0f, 1e-4f)) {
        std::cerr << "[FAIL] Transform Euler angles\n";
        return false;
    }

    std::cout << "[MathTest] Transform basics passed.\n";
    return true;
}

// ==================== MathUtils 测试 ====================

bool TestMathUtils()
{
    std::cout << "[MathTest] MathUtils...\n";

    // Clamp
    if (!ApproxEqual(TE::Math::Clamp(5.0f, 0.0f, 10.0f), 5.0f) ||
        !ApproxEqual(TE::Math::Clamp(-5.0f, 0.0f, 10.0f), 0.0f) ||
        !ApproxEqual(TE::Math::Clamp(15.0f, 0.0f, 10.0f), 10.0f)) {
        std::cerr << "[FAIL] Math::Clamp\n";
        return false;
    }

    // Lerp
    if (!ApproxEqual(TE::Math::Lerp(0.0f, 10.0f, 0.5f), 5.0f)) {
        std::cerr << "[FAIL] Math::Lerp\n";
        return false;
    }

    // Deg/Rad conversion
    if (!ApproxEqual(TE::Math::DegToRad(180.0f), TE::Math::PI) ||
        !ApproxEqual(TE::Math::RadToDeg(TE::Math::PI), 180.0f)) {
        std::cerr << "[FAIL] Math Deg/Rad conversion\n";
        return false;
    }

    // Min/Max
    if (TE::Math::Min(3.0f, 5.0f) != 3.0f || TE::Math::Max(3.0f, 5.0f) != 5.0f) {
        std::cerr << "[FAIL] Math::Min/Max\n";
        return false;
    }

    // Approximately
    if (!TE::Math::Approximately(1.0f, 1.000001f) ||
        TE::Math::Approximately(1.0f, 2.0f)) {
        std::cerr << "[FAIL] Math::Approximately\n";
        return false;
    }

    // SmoothStep - Hermite 插值，在 t=0.5 时结果应为 0.5
    // 但曲线形状是平滑的（导数为 0 在边界）
    float ss = TE::Math::SmoothStep(0.0f, 1.0f, 0.5f);
    if (!ApproxEqual(ss, 0.5f)) {
        std::cerr << "[FAIL] Math::SmoothStep at 0.5\n";
        return false;
    }
    // 在 t=0.25 时，结果应该小于 0.25（曲线起始更平缓）
    float ss25 = TE::Math::SmoothStep(0.0f, 1.0f, 0.25f);
    if (ss25 >= 0.25f || ss25 <= 0.0f) {
        std::cerr << "[FAIL] Math::SmoothStep at 0.25\n";
        return false;
    }

    std::cout << "[MathTest] MathUtils passed.\n";
    return true;
}

// ==================== Color 测试 ====================

bool TestColor()
{
    std::cout << "[MathTest] Color...\n";

    // RGB constructor
    TE::Color red(1.0f, 0.0f, 0.0f);
    if (!ApproxEqual(red.R, 1.0f) || !ApproxEqual(red.G, 0.0f) || !ApproxEqual(red.B, 0.0f)) {
        std::cerr << "[FAIL] Color RGB constructor\n";
        return false;
    }

    // Vector4 conversion
    TE::Vector4 v4 = red.ToVector4();
    if (!ApproxEqual(v4.X, 1.0f) || !ApproxEqual(v4.Y, 0.0f)) {
        std::cerr << "[FAIL] Color ToVector4\n";
        return false;
    }

    // HSV conversion
    TE::Color blue = TE::Color::FromHSV(240.0f, 1.0f, 1.0f);
    if (blue.B <= blue.R || blue.B <= blue.G) {
        std::cerr << "[FAIL] Color FromHSV blue\n";
        return false;
    }

    TE::Vector3 hsv = blue.ToHSV();
    if (!ApproxEqual(hsv.X, 240.0f, 1.0f) ||
        !ApproxEqual(hsv.Y, 1.0f, 0.01f) ||
        !ApproxEqual(hsv.Z, 1.0f, 0.01f)) {
        std::cerr << "[FAIL] Color ToHSV\n";
        return false;
    }

    // Color Lerp
    TE::Color white(1.0f, 1.0f, 1.0f);
    TE::Color black(0.0f, 0.0f, 0.0f);
    TE::Color gray = TE::Color::Lerp(black, white, 0.5f);
    if (!ApproxEqual(gray.R, 0.5f) || !ApproxEqual(gray.G, 0.5f) || !ApproxEqual(gray.B, 0.5f)) {
        std::cerr << "[FAIL] Color Lerp\n";
        return false;
    }

    // Luminance
    TE::Color yellow(1.0f, 1.0f, 0.0f);
    float lum = yellow.Luminance();
    if (lum <= 0.0f || lum >= 1.0f) {
        std::cerr << "[FAIL] Color Luminance\n";
        return false;
    }

    std::cout << "[MathTest] Color passed.\n";
    return true;
}

// ==================== Random 测试 ====================

bool TestRandom()
{
    std::cout << "[MathTest] Random...\n";

    // Seed with fixed value for reproducibility
    TE::Random::Seed(12345);

    // Range test (statistical)
    float min = 10.0f, max = 20.0f;
    float sum = 0.0f;
    const int count = 1000;
    for (int i = 0; i < count; ++i) {
        float val = TE::Random::Range(min, max);
        if (val < min || val >= max) {
            std::cerr << "[FAIL] Random::Range out of bounds\n";
            return false;
        }
        sum += val;
    }
    float avg = sum / count;
    if (avg < 14.0f || avg > 16.0f) {
        std::cerr << "[FAIL] Random::Range average\n";
        return false;
    }

    // Unit vector test
    for (int i = 0; i < 10; ++i) {
        TE::Vector3 unit = TE::Random::UnitVector();
        float len = unit.Length();
        if (!ApproxEqual(len, 1.0f, 1e-4f)) {
            std::cerr << "[FAIL] Random::UnitVector not unit length\n";
            return false;
        }
    }

    // Bool probability test
    int trueCount = 0;
    for (int i = 0; i < 1000; ++i) {
        if (TE::Random::Bool(0.7f)) trueCount++;
    }
    float trueRate = static_cast<float>(trueCount) / 1000.0f;
    if (trueRate < 0.6f || trueRate > 0.8f) {
        std::cerr << "[FAIL] Random::Bool probability\n";
        return false;
    }

    // Inside sphere test
    for (int i = 0; i < 10; ++i) {
        TE::Vector3 inside = TE::Random::InsideSphere(5.0f);
        if (inside.Length() > 5.0f + 1e-4f) {
            std::cerr << "[FAIL] Random::InsideSphere out of bounds\n";
            return false;
        }
    }

    std::cout << "[MathTest] Random passed.\n";
    return true;
}

// ==================== Geometry 测试 ====================

bool TestGeometry()
{
    std::cout << "[MathTest] Geometry...\n";

    // Ray
    TE::Ray ray(TE::Vector3::Zero, TE::Vector3::Forward);
    TE::Vector3 point = ray.GetPoint(5.0f);
    if (!ApproxEqual(point, TE::Vector3(0.0f, 0.0f, 5.0f))) {
        std::cerr << "[FAIL] Ray GetPoint\n";
        return false;
    }

    // Plane
    TE::Plane plane(TE::Vector3::Up, 0.0f); // y = 0 plane
    if (!plane.IsOnPlane(TE::Vector3(1.0f, 0.0f, 1.0f))) {
        std::cerr << "[FAIL] Plane IsOnPlane\n";
        return false;
    }
    if (!plane.IsInFront(TE::Vector3(0.0f, 1.0f, 0.0f))) {
        std::cerr << "[FAIL] Plane IsInFront\n";
        return false;
    }

    // Ray-Plane intersection
    TE::Ray downRay(TE::Vector3(0.0f, 5.0f, 0.0f), -TE::Vector3::Up);
    float dist;
    if (!plane.IntersectRay(downRay, dist)) {
        std::cerr << "[FAIL] Ray-Plane intersection miss\n";
        return false;
    }
    if (!ApproxEqual(dist, 5.0f)) {
        std::cerr << "[FAIL] Ray-Plane intersection distance\n";
        return false;
    }

    // BoundingBox
    TE::BoundingBox box(TE::Vector3(-1.0f, -1.0f, -1.0f), TE::Vector3(1.0f, 1.0f, 1.0f));
    if (!box.Contains(TE::Vector3::Zero)) {
        std::cerr << "[FAIL] BoundingBox Contains center\n";
        return false;
    }
    if (box.Contains(TE::Vector3(2.0f, 0.0f, 0.0f))) {
        std::cerr << "[FAIL] BoundingBox Contains outside\n";
        return false;
    }
    if (!ApproxEqual(box.GetCenter(), TE::Vector3::Zero)) {
        std::cerr << "[FAIL] BoundingBox GetCenter\n";
        return false;
    }

    // Box-Box intersection
    TE::BoundingBox box2(TE::Vector3(0.5f, 0.5f, 0.5f), TE::Vector3(2.0f, 2.0f, 2.0f));
    if (!box.Intersects(box2)) {
        std::cerr << "[FAIL] BoundingBox Intersects\n";
        return false;
    }

    // Ray-Box intersection
    TE::Ray boxRay(TE::Vector3(-5.0f, 0.0f, 0.0f), TE::Vector3::Right);
    float boxDist;
    if (!box.IntersectRay(boxRay, boxDist)) {
        std::cerr << "[FAIL] Ray-Box intersection miss\n";
        return false;
    }
    if (!ApproxEqual(boxDist, 4.0f)) {
        std::cerr << "[FAIL] Ray-Box intersection distance\n";
        return false;
    }

    // BoundingSphere
    TE::BoundingSphere sphere(TE::Vector3::Zero, 5.0f);
    if (!sphere.Contains(TE::Vector3(3.0f, 0.0f, 0.0f))) {
        std::cerr << "[FAIL] BoundingSphere Contains\n";
        return false;
    }
    if (sphere.Contains(TE::Vector3(6.0f, 0.0f, 0.0f))) {
        std::cerr << "[FAIL] BoundingSphere Contains outside\n";
        return false;
    }

    // Sphere-Sphere intersection
    TE::BoundingSphere sphere2(TE::Vector3(8.0f, 0.0f, 0.0f), 5.0f);
    if (!sphere.Intersects(sphere2)) {
        std::cerr << "[FAIL] BoundingSphere Intersects\n";
        return false;
    }

    std::cout << "[MathTest] Geometry passed.\n";
    return true;
}

// ==================== Bug 修复测试 ====================

bool TestBugFixes()
{
    std::cout << "[MathTest] Bug fixes...\n";

    // InverseLerp 除零保护
    float result = TE::Math::InverseLerp(5.0f, 5.0f, 5.0f);
    if (!ApproxEqual(result, 0.0f)) {
        std::cerr << "[FAIL] InverseLerp divide-by-zero protection\n";
        return false;
    }

    // InverseLerp 正常情况
    if (!ApproxEqual(TE::Math::InverseLerp(0.0f, 10.0f, 5.0f), 0.5f)) {
        std::cerr << "[FAIL] InverseLerp normal case\n";
        return false;
    }

    // SmoothStep 除零保护
    float ssResult = TE::Math::SmoothStep(3.0f, 3.0f, 3.0f);
    if (!ApproxEqual(ssResult, 0.0f)) {
        std::cerr << "[FAIL] SmoothStep divide-by-zero protection\n";
        return false;
    }

    // SmootherStep 除零保护
    float sssResult = TE::Math::SmootherStep(3.0f, 3.0f, 3.0f);
    if (!ApproxEqual(sssResult, 0.0f)) {
        std::cerr << "[FAIL] SmootherStep divide-by-zero protection\n";
        return false;
    }

    // Remap 除零保护
    float remapResult = TE::Math::Remap(5.0f, 5.0f, 5.0f, 0.0f, 100.0f);
    if (!ApproxEqual(remapResult, 0.0f)) {
        std::cerr << "[FAIL] Remap divide-by-zero protection\n";
        return false;
    }

    // Remap 正常情况
    if (!ApproxEqual(TE::Math::Remap(5.0f, 0.0f, 10.0f, 0.0f, 100.0f), 50.0f)) {
        std::cerr << "[FAIL] Remap normal case\n";
        return false;
    }

    // Color Equals
    TE::Color c1(0.5f, 0.5f, 0.5f, 1.0f);
    TE::Color c2(0.5f + 1e-7f, 0.5f, 0.5f, 1.0f);
    if (!c1.Equals(c2)) {
        std::cerr << "[FAIL] Color Equals with small delta\n";
        return false;
    }

    // Color uint8_t constructor
    TE::Color fromByte(static_cast<uint8_t>(255), static_cast<uint8_t>(128), static_cast<uint8_t>(0), static_cast<uint8_t>(255));
    if (!ApproxEqual(fromByte.R, 1.0f) || fromByte.G < 0.49f || fromByte.G > 0.51f) {
        std::cerr << "[FAIL] Color uint8_t constructor\n";
        return false;
    }

    std::cout << "[MathTest] Bug fixes passed.\n";
    return true;
}

// ==================== 向量扩展测试 ====================

bool TestVectorExtensions()
{
    std::cout << "[MathTest] Vector extensions...\n";

    // Vector2 Equals
    TE::Vector2 v2a(1.0f, 2.0f);
    TE::Vector2 v2b(1.0f + 1e-7f, 2.0f - 1e-7f);
    if (!v2a.Equals(v2b)) {
        std::cerr << "[FAIL] Vector2 Equals\n";
        return false;
    }

    // Vector2 Min/Max
    TE::Vector2 v2min = TE::Vector2::Min(TE::Vector2(1.0f, 4.0f), TE::Vector2(3.0f, 2.0f));
    if (!ApproxEqual(v2min.X, 1.0f) || !ApproxEqual(v2min.Y, 2.0f)) {
        std::cerr << "[FAIL] Vector2 Min\n";
        return false;
    }

    // Vector3 Equals
    TE::Vector3 v3a(1.0f, 2.0f, 3.0f);
    TE::Vector3 v3b(1.0f + 1e-7f, 2.0f, 3.0f - 1e-7f);
    if (!v3a.Equals(v3b)) {
        std::cerr << "[FAIL] Vector3 Equals\n";
        return false;
    }

    // Vector3 Angle
    TE::Vector3 right = TE::Vector3::Right;
    TE::Vector3 up = TE::Vector3::Up;
    float angle = TE::Vector3::Angle(right, up);
    if (!ApproxEqual(angle, TE::Math::HALF_PI, 1e-4f)) {
        std::cerr << "[FAIL] Vector3 Angle (90 degrees)\n";
        return false;
    }

    // Vector3 Angle — 同方向应为 0
    float zeroAngle = TE::Vector3::Angle(right, right);
    if (!ApproxEqual(zeroAngle, 0.0f, 1e-4f)) {
        std::cerr << "[FAIL] Vector3 Angle (same direction)\n";
        return false;
    }

    // Vector3 Min/Max
    TE::Vector3 v3min = TE::Vector3::Min(TE::Vector3(1.0f, 5.0f, 3.0f), TE::Vector3(4.0f, 2.0f, 6.0f));
    if (!ApproxEqual(v3min, TE::Vector3(1.0f, 2.0f, 3.0f))) {
        std::cerr << "[FAIL] Vector3 Min\n";
        return false;
    }

    TE::Vector3 v3max = TE::Vector3::Max(TE::Vector3(1.0f, 5.0f, 3.0f), TE::Vector3(4.0f, 2.0f, 6.0f));
    if (!ApproxEqual(v3max, TE::Vector3(4.0f, 5.0f, 6.0f))) {
        std::cerr << "[FAIL] Vector3 Max\n";
        return false;
    }

    // Vector3 ClampLength
    TE::Vector3 longVec(10.0f, 0.0f, 0.0f);
    TE::Vector3 clamped = longVec.ClampLength(5.0f);
    if (!ApproxEqual(clamped.Length(), 5.0f, 1e-4f)) {
        std::cerr << "[FAIL] Vector3 ClampLength\n";
        return false;
    }
    // 短向量不应被修改
    TE::Vector3 shortVec(1.0f, 0.0f, 0.0f);
    TE::Vector3 notClamped = shortVec.ClampLength(5.0f);
    if (!ApproxEqual(notClamped, shortVec)) {
        std::cerr << "[FAIL] Vector3 ClampLength (no clamp needed)\n";
        return false;
    }

    // Vector3 MoveTowards
    TE::Vector3 current(0.0f, 0.0f, 0.0f);
    TE::Vector3 target(10.0f, 0.0f, 0.0f);
    TE::Vector3 moved = TE::Vector3::MoveTowards(current, target, 3.0f);
    if (!ApproxEqual(moved, TE::Vector3(3.0f, 0.0f, 0.0f))) {
        std::cerr << "[FAIL] Vector3 MoveTowards\n";
        return false;
    }
    // 当距离小于 maxDelta 时应到达目标
    TE::Vector3 nearTarget(9.0f, 0.0f, 0.0f);
    TE::Vector3 reached = TE::Vector3::MoveTowards(nearTarget, target, 5.0f);
    if (!ApproxEqual(reached, target)) {
        std::cerr << "[FAIL] Vector3 MoveTowards (reach target)\n";
        return false;
    }

    // Vector4 Equals
    TE::Vector4 v4a(1.0f, 2.0f, 3.0f, 4.0f);
    TE::Vector4 v4b(1.0f + 1e-7f, 2.0f, 3.0f, 4.0f - 1e-7f);
    if (!v4a.Equals(v4b)) {
        std::cerr << "[FAIL] Vector4 Equals\n";
        return false;
    }

    // Quat Equals
    TE::Quat q1(0.0f, 0.7071f, 0.0f, 0.7071f);
    TE::Quat q2(0.0f, 0.7071f + 1e-7f, 0.0f, 0.7071f);
    if (!q1.Equals(q2)) {
        std::cerr << "[FAIL] Quat Equals\n";
        return false;
    }
    // q 和 -q 应视为相等旋转
    TE::Quat q3 = -q1;
    if (!q1.Equals(q3)) {
        std::cerr << "[FAIL] Quat Equals (q == -q)\n";
        return false;
    }

    std::cout << "[MathTest] Vector extensions passed.\n";
    return true;
}

// ==================== 矩阵分解测试 ====================

bool TestMatrixDecompose()
{
    std::cout << "[MathTest] Matrix decompose...\n";

    // Matrix3 Determinant
    TE::Matrix3 m3Identity;
    float det3 = m3Identity.Determinant();
    if (!ApproxEqual(det3, 1.0f)) {
        std::cerr << "[FAIL] Matrix3 Determinant (identity)\n";
        return false;
    }

    // Matrix4 Determinant
    float det4 = TE::Matrix4::Identity.Determinant();
    if (!ApproxEqual(det4, 1.0f)) {
        std::cerr << "[FAIL] Matrix4 Determinant (identity)\n";
        return false;
    }

    // Scale matrix determinant = sx * sy * sz
    TE::Matrix4 scaleMat = TE::Matrix4::Scale(TE::Vector3(2.0f, 3.0f, 4.0f));
    float scaleDet = scaleMat.Determinant();
    if (!ApproxEqual(scaleDet, 24.0f)) {
        std::cerr << "[FAIL] Matrix4 Determinant (scale)\n";
        return false;
    }

    // Matrix4 ToMatrix3
    TE::Matrix4 rot = TE::Matrix4::Rotate(TE::Math::PI / 4.0f, TE::Vector3::Up);
    TE::Matrix3 rot3 = rot.ToMatrix3();
    // 旋转矩阵的 3x3 部分行列式应接近 1
    float rot3Det = rot3.Determinant();
    if (!ApproxEqual(rot3Det, 1.0f, 1e-4f)) {
        std::cerr << "[FAIL] Matrix4 ToMatrix3 rotation determinant\n";
        return false;
    }

    // Matrix4 GetTranslation
    TE::Matrix4 transMat = TE::Matrix4::Translate(TE::Vector3(10.0f, 20.0f, 30.0f));
    TE::Vector3 trans = transMat.GetTranslation();
    if (!ApproxEqual(trans, TE::Vector3(10.0f, 20.0f, 30.0f))) {
        std::cerr << "[FAIL] Matrix4 GetTranslation\n";
        return false;
    }

    // Matrix4 GetScale
    TE::Vector3 scaleVec = scaleMat.GetScale();
    if (!ApproxEqual(scaleVec, TE::Vector3(2.0f, 3.0f, 4.0f))) {
        std::cerr << "[FAIL] Matrix4 GetScale\n";
        return false;
    }

    // Matrix4 Decompose — 组合一个 TRS 矩阵再分解
    TE::Vector3 expectedTrans(5.0f, 10.0f, 15.0f);
    TE::Vector3 expectedScale(2.0f, 3.0f, 4.0f);
    TE::Quat expectedRot(TE::Vector3::Up, TE::Math::PI / 6.0f); // 30 度绕 Y 旋转

    TE::Matrix4 trsMat = TE::Matrix4::Translate(expectedTrans) *
                          expectedRot.ToMatrix4() *
                          TE::Matrix4::Scale(expectedScale);

    TE::Vector3 decTrans, decScale;
    TE::Quat decRot;
    bool success = trsMat.Decompose(decTrans, decRot, decScale);
    if (!success) {
        std::cerr << "[FAIL] Matrix4 Decompose returned false\n";
        return false;
    }
    if (!ApproxEqual(decTrans, expectedTrans)) {
        std::cerr << "[FAIL] Matrix4 Decompose translation\n";
        return false;
    }
    if (!ApproxEqual(decScale, expectedScale, 1e-3f)) {
        std::cerr << "[FAIL] Matrix4 Decompose scale\n";
        return false;
    }

    // GetNormalMatrix — 正交旋转矩阵的法线矩阵应等于旋转矩阵本身
    TE::Matrix3 normalMat = rot.GetNormalMatrix();
    float normalDet = normalMat.Determinant();
    if (!ApproxEqual(normalDet, 1.0f, 1e-3f)) {
        std::cerr << "[FAIL] Matrix4 GetNormalMatrix determinant\n";
        return false;
    }

    std::cout << "[MathTest] Matrix decompose passed.\n";
    return true;
}

// ==================== Frustum 测试 ====================

bool TestFrustum()
{
    std::cout << "[MathTest] Frustum...\n";

    // 创建一个简单的视图投影矩阵
    TE::Matrix4 view = TE::Matrix4::LookAt(
        TE::Vector3(0.0f, 0.0f, 5.0f),   // eye
        TE::Vector3(0.0f, 0.0f, 0.0f),   // center
        TE::Vector3(0.0f, 1.0f, 0.0f)    // up
    );
    TE::Matrix4 proj = TE::Matrix4::Perspective(
        TE::Math::DegToRad(60.0f),  // fov
        1.0f,                         // aspect
        0.1f,                         // near
        100.0f                        // far
    );
    TE::Matrix4 vp = proj * view;

    TE::Frustum frustum = TE::Frustum::FromViewProjection(vp);

    // 原点应该在视锥体内（相机在 z=5 看向原点，原点在 near 和 far 之间）
    if (!frustum.ContainsPoint(TE::Vector3(0.0f, 0.0f, 0.0f))) {
        std::cerr << "[FAIL] Frustum ContainsPoint (origin)\n";
        return false;
    }

    // 相机后方的点应该不在视锥体内
    if (frustum.ContainsPoint(TE::Vector3(0.0f, 0.0f, 10.0f))) {
        std::cerr << "[FAIL] Frustum ContainsPoint (behind camera)\n";
        return false;
    }

    // 非常远的点应该不在视锥体内
    if (frustum.ContainsPoint(TE::Vector3(0.0f, 0.0f, -200.0f))) {
        std::cerr << "[FAIL] Frustum ContainsPoint (beyond far plane)\n";
        return false;
    }

    // AABB 相交测试 — 原点处的包围盒应与视锥体相交
    TE::BoundingBox boxInFrustum(TE::Vector3(-1.0f, -1.0f, -1.0f), TE::Vector3(1.0f, 1.0f, 1.0f));
    if (!frustum.IntersectsAABB(boxInFrustum)) {
        std::cerr << "[FAIL] Frustum IntersectsAABB (center box)\n";
        return false;
    }

    // 相机后方的包围盒不应与视锥体相交
    TE::BoundingBox boxBehind(TE::Vector3(-1.0f, -1.0f, 9.0f), TE::Vector3(1.0f, 1.0f, 11.0f));
    if (frustum.IntersectsAABB(boxBehind)) {
        std::cerr << "[FAIL] Frustum IntersectsAABB (behind camera)\n";
        return false;
    }

    // 球体相交测试 — 原点处的球应与视锥体相交
    TE::BoundingSphere sphereIn(TE::Vector3::Zero, 2.0f);
    if (!frustum.IntersectsSphere(sphereIn)) {
        std::cerr << "[FAIL] Frustum IntersectsSphere (center)\n";
        return false;
    }

    // 远处的球不应与视锥体相交
    TE::BoundingSphere sphereFar(TE::Vector3(0.0f, 0.0f, -500.0f), 1.0f);
    if (frustum.IntersectsSphere(sphereFar)) {
        std::cerr << "[FAIL] Frustum IntersectsSphere (far away)\n";
        return false;
    }

    std::cout << "[MathTest] Frustum passed.\n";
    return true;
}

// ==================== IntVector/Rect 测试 ====================

bool TestIntVectors()
{
    std::cout << "[MathTest] IntVector/Rect...\n";

    // IntVector2 基本运算
    TE::IntVector2 iv2a(3, 4);
    TE::IntVector2 iv2b(1, 2);
    TE::IntVector2 iv2sum = iv2a + iv2b;
    if (iv2sum.X != 4 || iv2sum.Y != 6) {
        std::cerr << "[FAIL] IntVector2 addition\n";
        return false;
    }

    // IntVector2 ToFloat
    TE::Vector2 fv2 = iv2a.ToFloat();
    if (!ApproxEqual(fv2.X, 3.0f) || !ApproxEqual(fv2.Y, 4.0f)) {
        std::cerr << "[FAIL] IntVector2 ToFloat\n";
        return false;
    }

    // IntVector2 FromFloat
    TE::IntVector2 fromFloat = TE::IntVector2::FromFloat(TE::Vector2(3.7f, 4.2f));
    if (fromFloat.X != 3 || fromFloat.Y != 4) {
        std::cerr << "[FAIL] IntVector2 FromFloat\n";
        return false;
    }

    // IntVector2 Min/Max
    TE::IntVector2 minIV = TE::IntVector2::Min(TE::IntVector2(1, 4), TE::IntVector2(3, 2));
    if (minIV.X != 1 || minIV.Y != 2) {
        std::cerr << "[FAIL] IntVector2 Min\n";
        return false;
    }

    // IntVector3 基本运算
    TE::IntVector3 iv3(1, 2, 3);
    TE::IntVector3 iv3scaled = iv3 * 2;
    if (iv3scaled.X != 2 || iv3scaled.Y != 4 || iv3scaled.Z != 6) {
        std::cerr << "[FAIL] IntVector3 scalar multiply\n";
        return false;
    }

    // IntVector3 常量
    if (TE::IntVector3::Zero.X != 0 || TE::IntVector3::One.X != 1) {
        std::cerr << "[FAIL] IntVector3 constants\n";
        return false;
    }

    // Rect 基本操作
    TE::Rect rect(10.0f, 20.0f, 100.0f, 50.0f);
    if (!ApproxEqual(rect.GetRight(), 110.0f) || !ApproxEqual(rect.GetBottom(), 70.0f)) {
        std::cerr << "[FAIL] Rect edges\n";
        return false;
    }

    // Rect Contains
    if (!rect.Contains(TE::Vector2(50.0f, 40.0f))) {
        std::cerr << "[FAIL] Rect Contains (inside)\n";
        return false;
    }
    if (rect.Contains(TE::Vector2(0.0f, 0.0f))) {
        std::cerr << "[FAIL] Rect Contains (outside)\n";
        return false;
    }

    // Rect Intersects
    TE::Rect rect2(50.0f, 40.0f, 200.0f, 100.0f);
    if (!rect.Intersects(rect2)) {
        std::cerr << "[FAIL] Rect Intersects\n";
        return false;
    }

    // Rect Intersection
    TE::Rect inter = rect.Intersection(rect2);
    if (!ApproxEqual(inter.X, 50.0f) || !ApproxEqual(inter.Y, 40.0f) ||
        !ApproxEqual(inter.Width, 60.0f) || !ApproxEqual(inter.Height, 30.0f)) {
        std::cerr << "[FAIL] Rect Intersection\n";
        return false;
    }

    // Rect FromMinMax
    TE::Rect rectMM = TE::Rect::FromMinMax(10.0f, 20.0f, 110.0f, 70.0f);
    if (rectMM != rect) {
        std::cerr << "[FAIL] Rect FromMinMax\n";
        return false;
    }

    // IntRect 基本操作
    TE::IntRect intRect(0, 0, 1920, 1080);
    if (intRect.GetArea() != 1920 * 1080) {
        std::cerr << "[FAIL] IntRect GetArea\n";
        return false;
    }

    // IntRect Contains
    if (!intRect.Contains(TE::IntVector2(960, 540))) {
        std::cerr << "[FAIL] IntRect Contains\n";
        return false;
    }
    if (intRect.Contains(TE::IntVector2(1920, 1080))) {
        // IntRect 使用半开区间 [min, max)
        std::cerr << "[FAIL] IntRect Contains (boundary)\n";
        return false;
    }

    // IntRect ToFloat
    TE::Rect floatRect = intRect.ToFloat();
    if (!ApproxEqual(floatRect.Width, 1920.0f) || !ApproxEqual(floatRect.Height, 1080.0f)) {
        std::cerr << "[FAIL] IntRect ToFloat\n";
        return false;
    }

    std::cout << "[MathTest] IntVector/Rect passed.\n";
    return true;
}

} // anonymous namespace

int main()
{
    std::cout << "========================================\n";
    std::cout << "ToyEngine Math Module Test\n";
    std::cout << "========================================\n\n";

    TE::MemoryInit();

    bool allPassed = true;

    allPassed &= TestVectorBasics();
    allPassed &= TestMatrixBasics();
    allPassed &= TestQuaternionBasics();
    allPassed &= TestTransformBasics();
    allPassed &= TestMathUtils();
    allPassed &= TestColor();
    allPassed &= TestRandom();
    allPassed &= TestGeometry();
    allPassed &= TestBugFixes();
    allPassed &= TestVectorExtensions();
    allPassed &= TestMatrixDecompose();
    allPassed &= TestFrustum();
    allPassed &= TestIntVectors();

    TE::MemoryShutdown();

    std::cout << "\n========================================\n";
    if (allPassed) {
        std::cout << "All tests PASSED!\n";
        return 0;
    } else {
        std::cout << "Some tests FAILED!\n";
        return 1;
    }
}
