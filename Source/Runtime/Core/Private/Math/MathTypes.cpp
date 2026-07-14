// ToyEngine Core Module
// 数学类型实现 - Vector、Matrix、Quat 的方法实现

#include "Math/Matrix.h"
#include "Math/Quat.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace TE {

namespace {

glm::vec3 ToGlm(const Vector3& v)
{
    return {v.X, v.Y, v.Z};
}

glm::mat3 ToGlm(const Matrix3& m)
{
    glm::mat3 result(1.0f);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result[i][j] = m.M[i][j];
        }
    }
    return result;
}

glm::mat4 ToGlm(const Matrix4& m)
{
    glm::mat4 result(1.0f);
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result[i][j] = m.M[i][j];
        }
    }
    return result;
}

glm::quat ToGlm(const Quat& q)
{
    return {q.W, q.X, q.Y, q.Z};
}

Matrix3 FromGlm(const glm::mat3& m)
{
    Matrix3 result(0.0f);
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result.M[i][j] = m[i][j];
        }
    }
    return result;
}

Matrix4 FromGlm(const glm::mat4& m)
{
    Matrix4 result(0.0f);
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            result.M[i][j] = m[i][j];
        }
    }
    return result;
}

Quat FromGlm(const glm::quat& q)
{
    return {q.x, q.y, q.z, q.w};
}

} // namespace

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
    glm::mat3 glmMat = ToGlm(*this);
    glm::mat3 inv = glm::inverse(glmMat);
    return FromGlm(inv);
}

// Matrix3 行列式
float Matrix3::Determinant() const
{
    glm::mat3 glmMat = ToGlm(*this);
    return glm::determinant(glmMat);
}

// ==================== Matrix4 常量 ====================
const Matrix4 Matrix4::Identity(1.0f);
const Matrix4 Matrix4::Zero(0.0f);

// Matrix4 逆矩阵
Matrix4 Matrix4::Inverse() const
{
    glm::mat4 glmMat = ToGlm(*this);
    glm::mat4 inv = glm::inverse(glmMat);
    return FromGlm(inv);
}

// Matrix4 行列式
float Matrix4::Determinant() const
{
    glm::mat4 glmMat = ToGlm(*this);
    return glm::determinant(glmMat);
}

// Matrix4 法线变换矩阵（逆转置 3x3）
Matrix3 Matrix4::GetNormalMatrix() const
{
    // 提取左上角 3x3，然后求逆转置
    Matrix3 upper = ToMatrix3();
    Matrix3 inv = upper.Inverse();
    return inv.Transpose();
}

// Matrix4 分解为 TRS 分量
bool Matrix4::Decompose(Vector3& outTranslation, Quat& outRotation, Vector3& outScale) const
{
    glm::mat4 glmMat = ToGlm(*this);
    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;

    bool success = glm::decompose(glmMat, scale, rotation, translation, skew, perspective);
    if (success)
    {
        outTranslation = Vector3(translation.x, translation.y, translation.z);
        outRotation = Quat(rotation.x, rotation.y, rotation.z, rotation.w);
        outScale = Vector3(scale.x, scale.y, scale.z);
    }
    return success;
}

// 获取平移分量
Vector3 Matrix4::GetTranslation() const
{
    return {M[3][0], M[3][1], M[3][2]};
}

// 获取缩放分量
Vector3 Matrix4::GetScale() const
{
    // 各列向量的长度即为缩放
    float sx = std::sqrt(M[0][0] * M[0][0] + M[0][1] * M[0][1] + M[0][2] * M[0][2]);
    float sy = std::sqrt(M[1][0] * M[1][0] + M[1][1] * M[1][1] + M[1][2] * M[1][2]);
    float sz = std::sqrt(M[2][0] * M[2][0] + M[2][1] * M[2][1] + M[2][2] * M[2][2]);
    return {sx, sy, sz};
}

// 获取旋转分量
Quat Matrix4::GetRotation() const
{
    Vector3 scale = GetScale();
    // 去缩放得到纯旋转矩阵
    float invSx = scale.X > 1e-6f ? 1.0f / scale.X : 0.0f;
    float invSy = scale.Y > 1e-6f ? 1.0f / scale.Y : 0.0f;
    float invSz = scale.Z > 1e-6f ? 1.0f / scale.Z : 0.0f;

    glm::mat3 rotMat;
    rotMat[0][0] = M[0][0] * invSx; rotMat[0][1] = M[0][1] * invSx; rotMat[0][2] = M[0][2] * invSx;
    rotMat[1][0] = M[1][0] * invSy; rotMat[1][1] = M[1][1] * invSy; rotMat[1][2] = M[1][2] * invSy;
    rotMat[2][0] = M[2][0] * invSz; rotMat[2][1] = M[2][1] * invSz; rotMat[2][2] = M[2][2] * invSz;

    glm::quat q = glm::quat_cast(rotMat);
    return {q.x, q.y, q.z, q.w};
}

// 提取左上角 3x3 矩阵
Matrix3 Matrix4::ToMatrix3() const
{
    Matrix3 result;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            result.M[i][j] = M[i][j];
    return result;
}

// Matrix4 变换辅助函数
Matrix4 Matrix4::Translate(const Vector3& translation)
{
    glm::mat4 result = glm::translate(glm::mat4(1.0f), ToGlm(translation));
    return FromGlm(result);
}

Matrix4 Matrix4::Rotate(float angleRadians, const Vector3& axis)
{
    glm::mat4 result = glm::rotate(glm::mat4(1.0f), angleRadians, ToGlm(axis));
    return FromGlm(result);
}

Matrix4 Matrix4::Scale(const Vector3& scale)
{
    glm::mat4 result = glm::scale(glm::mat4(1.0f), ToGlm(scale));
    return FromGlm(result);
}

Matrix4 Matrix4::LookAtRH(const Vector3& eye, const Vector3& center, const Vector3& up)
{
    glm::mat4 result = glm::lookAtRH(
        ToGlm(eye),
        ToGlm(center),
        ToGlm(up)
    );
    return FromGlm(result);
}

Matrix4 Matrix4::LookAtLH(const Vector3& eye, const Vector3& center, const Vector3& up)
{
    glm::mat4 result = glm::lookAtLH(
        ToGlm(eye),
        ToGlm(center),
        ToGlm(up)
    );
    return FromGlm(result);
}

Matrix4 Matrix4::PerspectiveRH_ZO(float fovRadians, float aspect, float nearPlane, float farPlane)
{
    glm::mat4 result = glm::perspectiveRH_ZO(fovRadians, aspect, nearPlane, farPlane);
    return FromGlm(result);
}

Matrix4 Matrix4::ReverseZProjectionZO(const Matrix4& projectionZO)
{
    Matrix4 reverseDepth(1.0f);
    reverseDepth(2, 2) = -1.0f;
    reverseDepth(3, 2) = 1.0f;
    return reverseDepth * projectionZO;
}

Matrix4 Matrix4::PerspectiveLH_ZO(float fovRadians, float aspect, float nearPlane, float farPlane)
{
    glm::mat4 result = glm::perspectiveLH_ZO(fovRadians, aspect, nearPlane, farPlane);
    return FromGlm(result);
}

Matrix4 Matrix4::PerspectiveRH_NO(float fovRadians, float aspect, float nearPlane, float farPlane)
{
    // glm::perspectiveNO 强制使用 [-1, 1] 深度范围（NO = Negative One to One）
    // 无论 GLM_FORCE_DEPTH_ZERO_TO_ONE 宏如何设置
    glm::mat4 result = glm::perspectiveRH_NO(fovRadians, aspect, nearPlane, farPlane);
    return FromGlm(result);
}

Matrix4 Matrix4::PerspectiveLH_NO(float fovRadians, float aspect, float nearPlane, float farPlane)
{
    glm::mat4 result = glm::perspectiveLH_NO(fovRadians, aspect, nearPlane, farPlane);
    return FromGlm(result);
}

Matrix4 Matrix4::OrthographicRH_ZO(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    glm::mat4 result = glm::orthoRH_ZO(left, right, bottom, top, nearPlane, farPlane);
    return FromGlm(result);
}

Matrix4 Matrix4::OrthographicLH_ZO(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    glm::mat4 result = glm::orthoLH_ZO(left, right, bottom, top, nearPlane, farPlane);
    return FromGlm(result);
}

// ==================== Quat 常量 ====================
const Quat Quat::Identity(0.0f, 0.0f, 0.0f, 1.0f);

// 从欧拉角构造
Quat Quat::FromEuler(float yaw, float pitch, float roll)
{
    // yaw(Y) then pitch(X) then roll(Z)
    const glm::quat yawQuat = glm::angleAxis(yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat pitchQuat = glm::angleAxis(pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::quat rollQuat = glm::angleAxis(roll, glm::vec3(0.0f, 0.0f, 1.0f));
    const glm::quat result = yawQuat * pitchQuat * rollQuat;
    return FromGlm(result);
}

// 转换为旋转矩阵
Matrix4 Quat::ToMatrix4() const
{
    glm::quat glmQuat = ToGlm(*this);
    glm::mat4 result = glm::mat4_cast(glmQuat);
    return FromGlm(result);
}

Matrix3 Quat::ToMatrix3() const
{
    glm::quat glmQuat = ToGlm(*this);
    glm::mat3 result = glm::mat3_cast(glmQuat);
    return FromGlm(result);
}

// 转换为欧拉角
Vector3 Quat::ToEulerAngles() const
{
    // 与 FromEuler 保持同一套约定：yaw(Y) * pitch(X) * roll(Z)。
    const Matrix3 matrix = Normalize().ToMatrix3();
    const float sinPitch = std::clamp(-matrix(2, 1), -1.0f, 1.0f);
    const float pitch = std::asin(sinPitch);
    const float cosPitch = std::cos(pitch);

    float yaw = 0.0f;
    float roll = 0.0f;
    if (std::abs(cosPitch) > 1e-5f)
    {
        yaw = std::atan2(matrix(2, 0), matrix(2, 2));
        roll = std::atan2(matrix(0, 1), matrix(1, 1));
    }
    else
    {
        // 万向节锁时 yaw 与 roll 耦合，固定 roll 为 0，保留朝向等价的 yaw。
        yaw = std::atan2(-matrix(0, 2), matrix(0, 0));
    }

    return {yaw, pitch, roll};
}

// 球面线性插值
Quat Quat::Slerp(const Quat& a, const Quat& b, float t)
{
    glm::quat glmA = ToGlm(a);
    glm::quat glmB = ToGlm(b);
    glm::quat result = glm::slerp(glmA, glmB, t);
    return FromGlm(result);
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
