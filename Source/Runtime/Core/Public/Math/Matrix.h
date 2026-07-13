// ToyEngine Core Module
// 矩阵类型定义

#pragma once

#include "Vector.h"

namespace TE {

struct Quat;

// ==================== Matrix3 ====================
struct [[nodiscard]] Matrix3
{
    // 3x3 矩阵，按列主序存储（与 glm 一致）
    float M[3][3]{};

    // 构造函数
    Matrix3()
    {
        // 初始化为单位矩阵
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                M[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    explicit Matrix3(float diagonal)
    {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                M[i][j] = (i == j) ? diagonal : 0.0f;
    }

    // 常量
    static const Matrix3 Identity;
    static const Matrix3 Zero;

    // 元素访问（列主序：M[column][row]）
    float& operator()(int col, int row) { return M[col][row]; }
    const float& operator()(int col, int row) const { return M[col][row]; }

    // 矩阵乘法
    Matrix3 operator*(const Matrix3& other) const
    {
        Matrix3 result(0.0f);
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 3; ++k)
                    result.M[i][j] += M[k][j] * other.M[i][k];
        return result;
    }

    Matrix3& operator*=(const Matrix3& other) { *this = *this * other; return *this; }

    Vector3 operator*(const Vector3& vec) const
    {
        return {
            M[0][0] * vec.X + M[1][0] * vec.Y + M[2][0] * vec.Z,
            M[0][1] * vec.X + M[1][1] * vec.Y + M[2][1] * vec.Z,
            M[0][2] * vec.X + M[1][2] * vec.Y + M[2][2] * vec.Z
        };
    }

    // 转置
    Matrix3 Transpose() const
    {
        Matrix3 result;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                result.M[i][j] = M[j][i];
        return result;
    }

    // 逆矩阵（使用 glm 实现）
    Matrix3 Inverse() const;

    /// <summary>
    /// 计算 3x3 矩阵行列式
    /// </summary>
    [[nodiscard]] float Determinant() const;

    // 获取原始数据指针（用于传递给图形 API）
    [[nodiscard]] const float* Data() const { return &M[0][0]; }
};

// ==================== Matrix4 ====================
struct [[nodiscard]] Matrix4
{
    // 4x4 矩阵，按列主序存储（与 glm 一致）
    float M[4][4]{};

    // 构造函数
    Matrix4()
    {
        // 初始化为单位矩阵
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                M[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    explicit Matrix4(float diagonal)
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                M[i][j] = (i == j) ? diagonal : 0.0f;
    }

    // 常量
    static const Matrix4 Identity;
    static const Matrix4 Zero;

    // 元素访问（列主序：M[column][row]）
    float& operator()(int col, int row) { return M[col][row]; }
    const float& operator()(int col, int row) const { return M[col][row]; }

    // 矩阵乘法
    Matrix4 operator*(const Matrix4& other) const
    {
        Matrix4 result(0.0f);
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    result.M[i][j] += M[k][j] * other.M[i][k];
        return result;
    }

    Matrix4& operator*=(const Matrix4& other) { *this = *this * other; return *this; }

    Vector4 operator*(const Vector4& vec) const
    {
        return {
            M[0][0] * vec.X + M[1][0] * vec.Y + M[2][0] * vec.Z + M[3][0] * vec.W,
            M[0][1] * vec.X + M[1][1] * vec.Y + M[2][1] * vec.Z + M[3][1] * vec.W,
            M[0][2] * vec.X + M[1][2] * vec.Y + M[2][2] * vec.Z + M[3][2] * vec.W,
            M[0][3] * vec.X + M[1][3] * vec.Y + M[2][3] * vec.Z + M[3][3] * vec.W
        };
    }

    // 转置
    Matrix4 Transpose() const
    {
        Matrix4 result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result.M[i][j] = M[j][i];
        return result;
    }

    // 逆矩阵（使用 glm 实现）
    Matrix4 Inverse() const;

    // 获取原始数据指针（用于传递给图形 API）
    [[nodiscard]] const float* Data() const { return &M[0][0]; }

    /// <summary>
    /// 计算 4x4 矩阵行列式
    /// </summary>
    [[nodiscard]] float Determinant() const;

    /// <summary>
    /// 获取法线变换矩阵（逆转置 3x3 矩阵）
    /// 在 PBR 渲染中用于将法线从模型空间变换到世界空间
    /// </summary>
    Matrix3 GetNormalMatrix() const;

    /// <summary>
    /// 从 TRS 矩阵分解出平移、旋转和缩放分量
    /// </summary>
    /// <returns>分解是否成功</returns>
    bool Decompose(Vector3& outTranslation, Quat& outRotation, Vector3& outScale) const;

    /// <summary>
    /// 快捷方法：获取平移分量（矩阵第 4 列）
    /// </summary>
    Vector3 GetTranslation() const;

    /// <summary>
    /// 快捷方法：获取缩放分量（各列向量长度）
    /// </summary>
    Vector3 GetScale() const;

    /// <summary>
    /// 快捷方法：获取旋转分量（去缩放后转为四元数）
    /// </summary>
    Quat GetRotation() const; // NOLINT(*-use-nodiscard)

    /// <summary>
    /// 提取左上角 3x3 旋转/缩放矩阵
    /// </summary>
    Matrix3 ToMatrix3() const;

    // 变换辅助函数
    static Matrix4 Translate(const Vector3& translation);
    static Matrix4 Rotate(float angleRadians, const Vector3& axis);
    static Matrix4 Scale(const Vector3& scale);
    /// 右手系 LookAt 视图矩阵。
    static Matrix4 LookAtRH(const Vector3& eye, const Vector3& center, const Vector3& up);

    /// 左手系 LookAt 视图矩阵。
    static Matrix4 LookAtLH(const Vector3& eye, const Vector3& center, const Vector3& up);

    /// 右手系透视投影，NDC 深度范围 [0, 1]（ZO = Zero To One）。
    static Matrix4 PerspectiveRH_ZO(float fovRadians, float aspect, float nearPlane, float farPlane);

    /// 左手系透视投影，NDC 深度范围 [0, 1]（ZO = Zero To One）。
    static Matrix4 PerspectiveLH_ZO(float fovRadians, float aspect, float nearPlane, float farPlane);

    /// 右手系透视投影，NDC 深度范围 [-1, 1]（NO = Negative One To One）。
    static Matrix4 PerspectiveRH_NO(float fovRadians, float aspect, float nearPlane, float farPlane);

    /// 左手系透视投影，NDC 深度范围 [-1, 1]（NO = Negative One To One）。
    static Matrix4 PerspectiveLH_NO(float fovRadians, float aspect, float nearPlane, float farPlane);

    /// 右手系正交投影，NDC 深度范围 [0, 1]（ZO = Zero To One）。
    static Matrix4 OrthographicRH_ZO(float left, float right, float bottom, float top, float nearPlane, float farPlane);

    /// 左手系正交投影，NDC 深度范围 [0, 1]（ZO = Zero To One）。
    static Matrix4 OrthographicLH_ZO(float left, float right, float bottom, float top, float nearPlane, float farPlane);
};


} // namespace TE
