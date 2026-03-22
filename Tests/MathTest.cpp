// ToyEngine - Math 模块完整测试
// 测试 MathTypes, Transform, MathUtils, Color, Random, Geometry

#include "Math/MathTypes.h"
#include "Math/Transform.h"
#include "Math/MathUtils.h"
#include "Math/Color.h"
#include "Math/Random.h"
#include "Math/Geometry.h"
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
