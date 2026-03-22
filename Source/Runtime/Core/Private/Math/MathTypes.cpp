// ToyEngine Core Module
// 数学类型实现 - MathTypes.h 的方法实现

#include "Math/MathTypes.h"

namespace TE {

// ==================== Vector2 常量 ====================
const Vector2 Vector2::Zero(0.0f, 0.0f);
const Vector2 Vector2::One(1.0f, 1.0f);
const Vector2 Vector2::Right(1.0f, 0.0f);
const Vector2 Vector2::Up(0.0f, 1.0f);

// ==================== Vector3 常量 ====================
const Vector3 Vector3::Zero(0.0f, 0.0f, 0.0f);
const Vector3 Vector3::One(1.0f, 1.0f, 1.0f);
const Vector3 Vector3::Right(1.0f, 0.0f, 0.0f);
const Vector3 Vector3::Up(0.0f, 1.0f, 0.0f);
const Vector3 Vector3::Forward(0.0f, 0.0f, 1.0f);

// ==================== Vector4 常量 ====================
const Vector4 Vector4::Zero(0.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4::One(1.0f, 1.0f, 1.0f, 1.0f);

// ==================== Matrix3 常量 ====================
const Matrix3 Matrix3::Identity(1.0f);
const Matrix3 Matrix3::Zero(0.0f);

// Matrix3 逆矩阵
Matrix3 Matrix3::Inverse() const
{
    glm::mat3 glmMat = static_cast<glm::mat3>(*this);
    glm::mat3 inv = glm::inverse(glmMat);
    return Matrix3(inv);
}

// ==================== Matrix4 常量 ====================
const Matrix4 Matrix4::Identity(1.0f);
const Matrix4 Matrix4::Zero(0.0f);

// Matrix4 逆矩阵
Matrix4 Matrix4::Inverse() const
{
    glm::mat4 glmMat = static_cast<glm::mat4>(*this);
    glm::mat4 inv = glm::inverse(glmMat);
    return Matrix4(inv);
}

// Matrix4 变换辅助函数
Matrix4 Matrix4::Translate(const Vector3& translation)
{
    glm::mat4 result = glm::translate(glm::mat4(1.0f), static_cast<glm::vec3>(translation));
    return Matrix4(result);
}

Matrix4 Matrix4::Rotate(float angleRadians, const Vector3& axis)
{
    glm::mat4 result = glm::rotate(glm::mat4(1.0f), angleRadians, static_cast<glm::vec3>(axis));
    return Matrix4(result);
}

Matrix4 Matrix4::Scale(const Vector3& scale)
{
    glm::mat4 result = glm::scale(glm::mat4(1.0f), static_cast<glm::vec3>(scale));
    return Matrix4(result);
}

Matrix4 Matrix4::LookAt(const Vector3& eye, const Vector3& center, const Vector3& up)
{
    glm::mat4 result = glm::lookAt(
        static_cast<glm::vec3>(eye),
        static_cast<glm::vec3>(center),
        static_cast<glm::vec3>(up)
    );
    return Matrix4(result);
}

Matrix4 Matrix4::Perspective(float fovRadians, float aspect, float nearPlane, float farPlane)
{
    glm::mat4 result = glm::perspective(fovRadians, aspect, nearPlane, farPlane);
    return Matrix4(result);
}

Matrix4 Matrix4::Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    glm::mat4 result = glm::ortho(left, right, bottom, top, nearPlane, farPlane);
    return Matrix4(result);
}

// ==================== Quat 常量 ====================
const Quat Quat::Identity(0.0f, 0.0f, 0.0f, 1.0f);

// 从欧拉角构造
Quat Quat::FromEuler(float yaw, float pitch, float roll)
{
    // yaw (Y), pitch (X), roll (Z)
    float cy = std::cos(yaw * 0.5f);
    float sy = std::sin(yaw * 0.5f);
    float cp = std::cos(pitch * 0.5f);
    float sp = std::sin(pitch * 0.5f);
    float cr = std::cos(roll * 0.5f);
    float sr = std::sin(roll * 0.5f);

    return Quat(
        sr * cp * cy - cr * sp * sy,  // X
        cr * sp * cy + sr * cp * sy,  // Y
        cr * cp * sy - sr * sp * cy,  // Z
        cr * cp * cy + sr * sp * sy   // W
    );
}

// 转换为旋转矩阵
Matrix4 Quat::ToMatrix4() const
{
    glm::quat glmQuat = static_cast<glm::quat>(*this);
    glm::mat4 result = glm::mat4_cast(glmQuat);
    return Matrix4(result);
}

Matrix3 Quat::ToMatrix3() const
{
    glm::quat glmQuat = static_cast<glm::quat>(*this);
    glm::mat3 result = glm::mat3_cast(glmQuat);
    return Matrix3(result);
}

// 转换为欧拉角
Vector3 Quat::ToEulerAngles() const
{
    // 归一化
    Quat q = Normalize();

    Vector3 angles;

    // sin(pitch)
    float sinp = 2.0f * (q.W * q.Y - q.Z * q.X);
    if (std::abs(sinp) >= 1.0f)
    {
        // 使用 90 度
        angles.Y = std::copysign(3.14159265359f / 2.0f, sinp);  // pitch
    }
    else
    {
        angles.Y = std::asin(sinp);  // pitch
    }

    // yaw
    angles.X = std::atan2(2.0f * (q.W * q.Z + q.X * q.Y), 1.0f - 2.0f * (q.Y * q.Y + q.Z * q.Z));

    // roll
    angles.Z = std::atan2(2.0f * (q.W * q.X + q.Y * q.Z), 1.0f - 2.0f * (q.X * q.X + q.Y * q.Y));

    return angles;
}

// 球面线性插值
Quat Quat::Slerp(const Quat& a, const Quat& b, float t)
{
    glm::quat glmA = static_cast<glm::quat>(a);
    glm::quat glmB = static_cast<glm::quat>(b);
    glm::quat result = glm::slerp(glmA, glmB, t);
    return Quat(result);
}

// 获取旋转轴和角度
void Quat::GetAxisAngle(Vector3& outAxis, float& outAngleRadians) const
{
    Quat q = Normalize();
    outAngleRadians = 2.0f * std::acos(q.W);
    float s = std::sqrt(1.0f - q.W * q.W);
    if (s < 0.001f)
    {
        outAxis = Vector3(q.X, q.Y, q.Z);
    }
    else
    {
        outAxis = Vector3(q.X / s, q.Y / s, q.Z / s);
    }
}

} // namespace TE
