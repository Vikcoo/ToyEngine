// ToyEngine Core Module
// 变换类 - 位置、旋转、缩放的组合

#pragma once

#include "MathTypes.h"

namespace TE {

/// <summary>
/// 变换类 - 表示 3D 空间中的位置、旋转和缩放
/// 使用四元数表示旋转，避免万向节锁
/// </summary>
class Transform
{
public:
    // 默认构造函数 - 单位变换
    Transform()
        : Position(Vector3::Zero)
        , Rotation(Quat::Identity)
        , Scale(Vector3::One)
    {}

    // 完整构造函数
    Transform(const Vector3& position, const Quat& rotation, const Vector3& scale)
        : Position(position)
        , Rotation(rotation)
        , Scale(scale)
    {}

    // 从位置构造（旋转和缩放使用默认值）
    explicit Transform(const Vector3& position)
        : Position(position)
        , Rotation(Quat::Identity)
        , Scale(Vector3::One)
    {}

    // 数据成员
    Vector3 Position;  // 位置
    Quat    Rotation;  // 旋转（四元数）
    Vector3 Scale;     // 缩放

    // ==================== 矩阵转换 ====================

    /// <summary>
    /// 转换为 4x4 变换矩阵
    /// 顺序：缩放 -> 旋转 -> 平移
    /// </summary>
    [[nodiscard]] Matrix4 ToMatrix() const;

    /// <summary>
    /// 从矩阵还原变换（缩放假设为正）
    /// </summary>
    [[nodiscard]] static Transform FromMatrix(const Matrix4& matrix);

    // ==================== 方向向量 ====================

    /// <summary>
    /// 获取前向向量（局部 Z 轴在世界空间的指向）
    /// </summary>
    [[nodiscard]] Vector3 GetForward() const;

    /// <summary>
    /// 获取右向向量（局部 X 轴在世界空间的指向）
    /// </summary>
    [[nodiscard]] Vector3 GetRight() const;

    /// <summary>
    /// 获取上向向量（局部 Y 轴在世界空间的指向）
    /// </summary>
    [[nodiscard]] Vector3 GetUp() const;

    /// <summary>
    /// 设置前向方向（保持上向量大致向上）
    /// </summary>
    void SetForward(const Vector3& forward);

    // ==================== 欧拉角支持 ====================

    /// <summary>
    /// 从欧拉角设置旋转（yaw, pitch, roll，单位：弧度）
    /// yaw: Y轴旋转，pitch: X轴旋转，roll: Z轴旋转
    /// </summary>
    void SetEulerAngles(float yaw, float pitch, float roll);
    void SetEulerAngles(const Vector3& eulerRadians);

    /// <summary>
    /// 获取欧拉角（yaw, pitch, roll，单位：弧度）
    /// </summary>
    [[nodiscard]] Vector3 GetEulerAngles() const;

    /// <summary>
    /// 从欧拉角创建变换（yaw, pitch, roll，单位：度）
    /// </summary>
    [[nodiscard]] static Transform FromEulerDegrees(float yawDeg, float pitchDeg, float rollDeg);

    /// <summary>
    /// 获取欧拉角（yaw, pitch, roll，单位：度）
    /// </summary>
    [[nodiscard]] Vector3 GetEulerAnglesDegrees() const;

    // ==================== LookAt ====================

    /// <summary>
    /// 使变换朝向目标点（保持上向量）
    /// </summary>
    void LookAt(const Vector3& target, const Vector3& worldUp = Vector3::Up);

    /// <summary>
    /// 创建 LookAt 变换（位置在 eye，朝向 center）
    /// </summary>
    [[nodiscard]] static Transform LookAt(const Vector3& eye, const Vector3& center, const Vector3& worldUp = Vector3::Up);

    // ==================== 变换操作 ====================

    /// <summary>
    /// 平移
    /// </summary>
    void Translate(const Vector3& delta);
    void Translate(float x, float y, float z);

    /// <summary>
    /// 绕世界轴旋转
    /// </summary>
    void Rotate(const Vector3& axis, float angleRadians);
    void RotateWorldX(float angleRadians);
    void RotateWorldY(float angleRadians);
    void RotateWorldZ(float angleRadians);

    /// <summary>
    /// 绕局部轴旋转
    /// </summary>
    void RotateLocalX(float angleRadians);
    void RotateLocalY(float angleRadians);
    void RotateLocalZ(float angleRadians);

    /// <summary>
    /// 缩放
    /// </summary>
    void SetUniformScale(float uniformScale);

    // ==================== 逆变换 ====================

    /// <summary>
    /// 获取逆变换
    /// </summary>
    [[nodiscard]] Transform Inverse() const;

    // ==================== 变换组合 ====================

    /// <summary>
    /// 组合两个变换（先应用 other，再应用 this）
    /// 结果 = this * other
    /// </summary>
    [[nodiscard]] Transform operator*(const Transform& other) const;

    [[nodiscard]] Transform& operator*=(const Transform& other);

    /// <summary>
    /// 对点应用变换
    /// </summary>
    [[nodiscard]] Vector3 TransformPoint(const Vector3& point) const;

    /// <summary>
    /// 对向量应用变换（忽略平移）
    /// </summary>
    [[nodiscard]] Vector3 TransformVector(const Vector3& vector) const;

    /// <summary>
    /// 对方向应用变换（仅旋转，忽略平移和缩放）
    /// </summary>
    [[nodiscard]] Vector3 TransformDirection(const Vector3& direction) const;

    /// <summary>
    /// 逆变换点
    /// </summary>
    [[nodiscard]] Vector3 InverseTransformPoint(const Vector3& point) const;

    /// <summary>
    /// 逆变换向量
    /// </summary>
    [[nodiscard]] Vector3 InverseTransformVector(const Vector3& vector) const;

    /// <summary>
    /// 逆变换方向
    /// </summary>
    [[nodiscard]] Vector3 InverseTransformDirection(const Vector3& direction) const;

    // ==================== 插值 ====================

    /// <summary>
    /// 线性插值位置，球面插值旋转，线性插值缩放
    /// </summary>
    [[nodiscard]] static Transform Lerp(const Transform& a, const Transform& b, float t);

    // ==================== 常量 ====================

    static const Transform Identity;
};

} // namespace TE
