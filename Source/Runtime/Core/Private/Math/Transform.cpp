// ToyEngine Core Module
// 变换类实现

#include "Math/Transform.h"

namespace TE {

// 常量定义
const Transform Transform::Identity = Transform();

namespace {

bool BuildRotationFromForward(const Vector3& forward, const Vector3& worldUp, Quat& outRotation)
{
    if (forward.LengthSquared() <= 1e-8f)
    {
        return false;
    }

    const Vector3 forwardNorm = forward.Normalize();
    Vector3 up = worldUp.LengthSquared() > 1e-8f ? worldUp.Normalize() : Vector3::Up;
    if (std::abs(Vector3::Dot(forwardNorm, up)) > 0.999f)
    {
        up = std::abs(Vector3::Dot(forwardNorm, Vector3::Up)) > 0.999f
            ? Vector3::Right
            : Vector3::Up;
    }

    const Vector3 right = Vector3::Cross(up, forwardNorm).Normalize();
    const Vector3 correctedUp = Vector3::Cross(forwardNorm, right).Normalize();

    // 列主序：三列分别是局部 X/Y/Z 轴在世界空间中的方向。
    Matrix4 rotMatrix(1.0f);
    rotMatrix(0, 0) = right.X;       rotMatrix(0, 1) = right.Y;       rotMatrix(0, 2) = right.Z;
    rotMatrix(1, 0) = correctedUp.X; rotMatrix(1, 1) = correctedUp.Y; rotMatrix(1, 2) = correctedUp.Z;
    rotMatrix(2, 0) = forwardNorm.X; rotMatrix(2, 1) = forwardNorm.Y; rotMatrix(2, 2) = forwardNorm.Z;

    outRotation = Transform::FromMatrix(rotMatrix).Rotation.Normalize();
    return true;
}

} // namespace

// 转换为 4x4 变换矩阵
Matrix4 Transform::ToMatrix() const
{
    // 顺序：缩放 -> 旋转 -> 平移
    Matrix4 scaleMatrix = Matrix4::Scale(Scale);
    Matrix4 rotationMatrix = Rotation.ToMatrix4();
    Matrix4 translationMatrix = Matrix4::Translate(Position);

    return translationMatrix * rotationMatrix * scaleMatrix;
}

// 从矩阵还原变换
Transform Transform::FromMatrix(const Matrix4& matrix)
{
    Transform result;

    // 提取位置（第四列）
    result.Position = {matrix(3, 0), matrix(3, 1), matrix(3, 2)};

    // 提取缩放（各列向量的长度）
    Vector3 col0(matrix(0, 0), matrix(0, 1), matrix(0, 2));
    Vector3 col1(matrix(1, 0), matrix(1, 1), matrix(1, 2));
    Vector3 col2(matrix(2, 0), matrix(2, 1), matrix(2, 2));

    result.Scale = Vector3(col0.Length(), col1.Length(), col2.Length());

    // 移除缩放，提取旋转
    if (result.Scale.X > 0.0f) col0 = col0 / result.Scale.X;
    if (result.Scale.Y > 0.0f) col1 = col1 / result.Scale.Y;
    if (result.Scale.Z > 0.0f) col2 = col2 / result.Scale.Z;

    // 从旋转矩阵构造四元数
    // 使用 glm 的算法
    float trace = col0.X + col1.Y + col2.Z;

    if (trace > 0.0f)
    {
        float s = 0.5f / std::sqrt(trace + 1.0f);
        result.Rotation.W = 0.25f / s;
        result.Rotation.X = (col1.Z - col2.Y) * s;
        result.Rotation.Y = (col2.X - col0.Z) * s;
        result.Rotation.Z = (col0.Y - col1.X) * s;
    }
    else if (col0.X > col1.Y && col0.X > col2.Z)
    {
        float s = 2.0f * std::sqrt(1.0f + col0.X - col1.Y - col2.Z);
        result.Rotation.W = (col1.Z - col2.Y) / s;
        result.Rotation.X = 0.25f * s;
        result.Rotation.Y = (col1.X + col0.Y) / s;
        result.Rotation.Z = (col2.X + col0.Z) / s;
    }
    else if (col1.Y > col2.Z)
    {
        float s = 2.0f * std::sqrt(1.0f + col1.Y - col0.X - col2.Z);
        result.Rotation.W = (col2.X - col0.Z) / s;
        result.Rotation.X = (col1.X + col0.Y) / s;
        result.Rotation.Y = 0.25f * s;
        result.Rotation.Z = (col2.Y + col1.Z) / s;
    }
    else
    {
        float s = 2.0f * std::sqrt(1.0f + col2.Z - col0.X - col1.Y);
        result.Rotation.W = (col0.Y - col1.X) / s;
        result.Rotation.X = (col2.X + col0.Z) / s;
        result.Rotation.Y = (col2.Y + col1.Z) / s;
        result.Rotation.Z = 0.25f * s;
    }

    return result;
}

// 获取前向向量
Vector3 Transform::GetForward() const
{
    // 局部 Z 轴（前向）在世界空间的指向
    return Rotation * Vector3::Forward;
}

// 获取右向向量
Vector3 Transform::GetRight() const
{
    // 局部 X 轴（右向）在世界空间的指向
    return Rotation * Vector3::Right;
}

// 获取上向向量
Vector3 Transform::GetUp() const
{
    // 局部 Y 轴（上向）在世界空间的指向
    return Rotation * Vector3::Up;
}

// 设置前向方向
void Transform::SetForward(const Vector3& forward)
{
    Quat rotation;
    if (BuildRotationFromForward(forward, Vector3::Up, rotation))
    {
        Rotation = rotation;
    }
}

// 欧拉角设置
void Transform::SetEulerAngles(float yaw, float pitch, float roll)
{
    Rotation = Quat::FromEuler(yaw, pitch, roll);
}

void Transform::SetEulerAngles(const Vector3& eulerRadians)
{
    SetEulerAngles(eulerRadians.X, eulerRadians.Y, eulerRadians.Z);
}

// 获取欧拉角
Vector3 Transform::GetEulerAngles() const
{
    return Rotation.ToEulerAngles();
}

// 从欧拉角（度）创建
Transform Transform::FromEulerDegrees(float yawDeg, float pitchDeg, float rollDeg)
{
    const float deg2rad = 3.14159265359f / 180.0f;
    Transform result;
    result.SetEulerAngles(yawDeg * deg2rad, pitchDeg * deg2rad, rollDeg * deg2rad);
    return result;
}

// 获取欧拉角（度）
Vector3 Transform::GetEulerAnglesDegrees() const
{
    const float rad2deg = 180.0f / 3.14159265359f;
    Vector3 rad = GetEulerAngles();
    return {rad.X * rad2deg, rad.Y * rad2deg, rad.Z * rad2deg};
}

// LookAt
void Transform::LookAt(const Vector3& target, const Vector3& worldUp)
{
    Quat rotation;
    if (BuildRotationFromForward(target - Position, worldUp, rotation))
    {
        Rotation = rotation;
    }
}

// 静态 LookAt
Transform Transform::LookAt(const Vector3& eye, const Vector3& center, const Vector3& worldUp)
{
    Transform result;
    result.Position = eye;
    Quat rotation;
    if (BuildRotationFromForward(center - eye, worldUp, rotation))
    {
        result.Rotation = rotation;
    }
    return result;
}

// 平移
void Transform::Translate(const Vector3& delta)
{
    Position += delta;
}

void Transform::Translate(float x, float y, float z)
{
    Position += {x, y, z};
}

// 绕世界轴旋转
void Transform::Rotate(const Vector3& axis, float angleRadians)
{
    Quat rotation(axis, angleRadians);
    Rotation = rotation * Rotation;
}

void Transform::RotateWorldX(float angleRadians)
{
    Rotate(Vector3::Right, angleRadians);
}

void Transform::RotateWorldY(float angleRadians)
{
    Rotate(Vector3::Up, angleRadians);
}

void Transform::RotateWorldZ(float angleRadians)
{
    Rotate(Vector3::Forward, angleRadians);
}

// 绕局部轴旋转
void Transform::RotateLocalX(float angleRadians)
{
    Quat rotation(GetRight(), angleRadians);
    Rotation = Rotation * rotation;
}

void Transform::RotateLocalY(float angleRadians)
{
    Quat rotation(GetUp(), angleRadians);
    Rotation = Rotation * rotation;
}

void Transform::RotateLocalZ(float angleRadians)
{
    Quat rotation(GetForward(), angleRadians);
    Rotation = Rotation * rotation;
}

// 统一缩放
void Transform::SetUniformScale(float uniformScale)
{
    Scale = {uniformScale, uniformScale, uniformScale};
}

// 逆变换
Transform Transform::Inverse() const
{
    Transform result;

    // 逆缩放
    result.Scale = {
        Scale.X != 0.0f ? 1.0f / Scale.X : 0.0f,
        Scale.Y != 0.0f ? 1.0f / Scale.Y : 0.0f,
        Scale.Z != 0.0f ? 1.0f / Scale.Z : 0.0f
    };

    // 逆旋转
    result.Rotation = Rotation.Conjugate();

    // 逆位置（先逆缩放，再逆旋转）
    Vector3 invPos = -Position;
    invPos = invPos * result.Scale;
    result.Position = result.Rotation * invPos;

    return result;
}

// 组合变换
Transform Transform::operator*(const Transform& other) const
{
    Transform result;

    // 组合位置：先应用 other 的缩放和旋转，再加上 other 的位置，最后应用 this 的变换
    result.Position = Position + Rotation * (Scale * other.Position);

    // 组合旋转：先 other 后 this
    result.Rotation = Rotation * other.Rotation;

    // 组合缩放
    result.Scale = Scale * other.Scale;

    return result;
}

Transform& Transform::operator*=(const Transform& other)
{
    *this = *this * other;
    return *this;
}

// 对点应用变换
Vector3 Transform::TransformPoint(const Vector3& point) const
{
    // 缩放 -> 旋转 -> 平移
    return Position + Rotation * (Scale * point);
}

// 对向量应用变换（忽略平移）
Vector3 Transform::TransformVector(const Vector3& vector) const
{
    // 缩放 -> 旋转
    return Rotation * (Scale * vector);
}

// 对方向应用变换（仅旋转）
Vector3 Transform::TransformDirection(const Vector3& direction) const
{
    return Rotation * direction;
}

// 逆变换点
Vector3 Transform::InverseTransformPoint(const Vector3& point) const
{
    // 逆平移 -> 逆旋转 -> 逆缩放
    Vector3 localPos = point - Position;
    localPos = Rotation.Conjugate() * localPos;
    return {
        Scale.X != 0.0f ? localPos.X / Scale.X : 0.0f,
        Scale.Y != 0.0f ? localPos.Y / Scale.Y : 0.0f,
        Scale.Z != 0.0f ? localPos.Z / Scale.Z : 0.0f
    };
}

// 逆变换向量
Vector3 Transform::InverseTransformVector(const Vector3& vector) const
{
    // 逆旋转 -> 逆缩放
    Vector3 localVec = Rotation.Conjugate() * vector;
    return {
        Scale.X != 0.0f ? localVec.X / Scale.X : 0.0f,
        Scale.Y != 0.0f ? localVec.Y / Scale.Y : 0.0f,
        Scale.Z != 0.0f ? localVec.Z / Scale.Z : 0.0f
    };
}

// 逆变换方向
Vector3 Transform::InverseTransformDirection(const Vector3& direction) const
{
    return Rotation.Conjugate() * direction;
}

// 插值
Transform Transform::Lerp(const Transform& a, const Transform& b, float t)
{
    Transform result;
    result.Position = Vector3::Lerp(a.Position, b.Position, t);
    result.Rotation = Quat::Slerp(a.Rotation, b.Rotation, t);
    result.Scale = Vector3::Lerp(a.Scale, b.Scale, t);
    return result;
}

} // namespace TE
