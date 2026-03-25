// ToyEngine Core Module
// 数学类型定义 - 封装 glm，提供引擎级数学 API

#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <algorithm>

namespace TE {

// 前向声明
struct Vector2;
struct Vector3;
struct Vector4;
struct Matrix3;
struct Matrix4;
struct Quat;

// ==================== Vector2 ====================
struct [[nodiscard]] Vector2
{
    float X, Y;

    // 构造函数
    Vector2() : X(0.0f), Y(0.0f) {}
    Vector2(float x, float y) : X(x), Y(y) {}
    explicit Vector2(float scalar) : X(scalar), Y(scalar) {}

    // 从 glm 构造/转换
    explicit Vector2(const glm::vec2& v) : X(v.x), Y(v.y) {}
    operator glm::vec2() const { return {X, Y}; }

    // 常量
    static const Vector2 Zero;
    static const Vector2 One;
    static const Vector2 Right;
    static const Vector2 Up;

    // 运算符重载
    Vector2 operator+(const Vector2& other) const { return {X + other.X, Y + other.Y}; }
    Vector2 operator-(const Vector2& other) const { return {X - other.X, Y - other.Y}; }
    Vector2 operator*(const Vector2& other) const { return {X * other.X, Y * other.Y}; }
    Vector2 operator/(const Vector2& other) const { return {X / other.X, Y / other.Y}; }
    Vector2 operator*(float scalar) const { return {X * scalar, Y * scalar}; }
    Vector2 operator/(float scalar) const { return {X / scalar, Y / scalar}; }
    Vector2 operator-() const { return {-X, -Y}; }

    Vector2& operator+=(const Vector2& other) { X += other.X; Y += other.Y; return *this; }
    Vector2& operator-=(const Vector2& other) { X -= other.X; Y -= other.Y; return *this; }
    Vector2& operator*=(const Vector2& other) { X *= other.X; Y *= other.Y; return *this; }
    Vector2& operator/=(const Vector2& other) { X /= other.X; Y /= other.Y; return *this; }
    Vector2& operator*=(float scalar) { X *= scalar; Y *= scalar; return *this; }
    Vector2& operator/=(float scalar) { X /= scalar; Y /= scalar; return *this; }

    bool operator==(const Vector2& other) const { return X == other.X && Y == other.Y; }
    bool operator!=(const Vector2& other) const { return !(*this == other); }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y; }

    Vector2 Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Vector2{X / len, Y / len} : Vector2::Zero;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; }
    }

    static float Dot(const Vector2& a, const Vector2& b) { return a.X * b.X + a.Y * b.Y; }
    static float Distance(const Vector2& a, const Vector2& b) { return (a - b).Length(); }
    static float DistanceSquared(const Vector2& a, const Vector2& b) { return (a - b).LengthSquared(); }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// </summary>
    [[nodiscard]] bool Equals(const Vector2& other, float epsilon = 1e-6f) const
    {
        return std::abs(X - other.X) <= epsilon && std::abs(Y - other.Y) <= epsilon;
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static Vector2 Min(const Vector2& a, const Vector2& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static Vector2 Max(const Vector2& a, const Vector2& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y)};
    }

    // 线性插值
    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t)
    {
        return a + (b - a) * t;
    }
};

// 标量乘法（左操作数）
inline Vector2 operator*(float scalar, const Vector2& vec) { return vec * scalar; }

// ==================== Vector3 ====================
struct [[nodiscard]] Vector3
{
    float X, Y, Z;

    // 构造函数
    Vector3() : X(0.0f), Y(0.0f), Z(0.0f) {}
    Vector3(float x, float y, float z) : X(x), Y(y), Z(z) {}
    explicit Vector3(float scalar) : X(scalar), Y(scalar), Z(scalar) {}

    // 从 glm 构造/转换
    explicit Vector3(const glm::vec3& v) : X(v.x), Y(v.y), Z(v.z) {}
    operator glm::vec3() const { return {X, Y, Z}; }

    // 常量
    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 Right;
    static const Vector3 Up;
    static const Vector3 Forward;

    // 运算符重载
    Vector3 operator+(const Vector3& other) const { return {X + other.X, Y + other.Y, Z + other.Z}; }
    Vector3 operator-(const Vector3& other) const { return {X - other.X, Y - other.Y, Z - other.Z}; }
    Vector3 operator*(const Vector3& other) const { return {X * other.X, Y * other.Y, Z * other.Z}; }
    Vector3 operator/(const Vector3& other) const { return {X / other.X, Y / other.Y, Z / other.Z}; }
    Vector3 operator*(float scalar) const { return {X * scalar, Y * scalar, Z * scalar}; }
    Vector3 operator/(float scalar) const { return {X / scalar, Y / scalar, Z / scalar}; }
    Vector3 operator-() const { return {-X, -Y, -Z}; }

    Vector3& operator+=(const Vector3& other) { X += other.X; Y += other.Y; Z += other.Z; return *this; }
    Vector3& operator-=(const Vector3& other) { X -= other.X; Y -= other.Y; Z -= other.Z; return *this; }
    Vector3& operator*=(const Vector3& other) { X *= other.X; Y *= other.Y; Z *= other.Z; return *this; }
    Vector3& operator/=(const Vector3& other) { X /= other.X; Y /= other.Y; Z /= other.Z; return *this; }
    Vector3& operator*=(float scalar) { X *= scalar; Y *= scalar; Z *= scalar; return *this; }
    Vector3& operator/=(float scalar) { X /= scalar; Y /= scalar; Z /= scalar; return *this; }

    bool operator==(const Vector3& other) const { return X == other.X && Y == other.Y && Z == other.Z; }
    bool operator!=(const Vector3& other) const { return !(*this == other); }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y + Z * Z); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y + Z * Z; }

    Vector3 Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Vector3{X / len, Y / len, Z / len} : Vector3::Zero;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; Z /= len; }
    }

    static float Dot(const Vector3& a, const Vector3& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z; }
    static Vector3 Cross(const Vector3& a, const Vector3& b)
    {
        return {
            a.Y * b.Z - a.Z * b.Y,
            a.Z * b.X - a.X * b.Z,
            a.X * b.Y - a.Y * b.X
        };
    }
    static float Distance(const Vector3& a, const Vector3& b) { return (a - b).Length(); }
    static float DistanceSquared(const Vector3& a, const Vector3& b) { return (a - b).LengthSquared(); }

    // 线性插值
    static Vector3 Lerp(const Vector3& a, const Vector3& b, float t)
    {
        return a + (b - a) * t;
    }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// </summary>
    [[nodiscard]] bool Equals(const Vector3& other, float epsilon = 1e-6f) const
    {
        return std::abs(X - other.X) <= epsilon &&
               std::abs(Y - other.Y) <= epsilon &&
               std::abs(Z - other.Z) <= epsilon;
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static Vector3 Min(const Vector3& a, const Vector3& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static Vector3 Max(const Vector3& a, const Vector3& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z)};
    }

    /// <summary>
    /// 计算两个向量之间的夹角（弧度）
    /// </summary>
    static float Angle(const Vector3& a, const Vector3& b)
    {
        float lenProduct = a.Length() * b.Length();
        if (lenProduct < 1e-6f)
            return 0.0f;
        float dot = Dot(a, b) / lenProduct;
        // Clamp 以避免 acos 参数超出 [-1, 1]
        dot = std::max(-1.0f, std::min(1.0f, dot));
        return std::acos(dot);
    }

    /// <summary>
    /// 限制向量长度不超过 maxLength
    /// </summary>
    Vector3 ClampLength(float maxLength) const
    {
        float lenSq = LengthSquared();
        if (lenSq > maxLength * maxLength)
        {
            float len = std::sqrt(lenSq);
            return *this * (maxLength / len);
        }
        return *this;
    }

    /// <summary>
    /// 从当前位置向目标移动，每次最多移动 maxDelta 距离
    /// </summary>
    static Vector3 MoveTowards(const Vector3& current, const Vector3& target, float maxDelta)
    {
        Vector3 diff = target - current;
        float dist = diff.Length();
        if (dist <= maxDelta || dist < 1e-6f)
            return target;
        return current + diff * (maxDelta / dist);
    }

    /// <summary>
    /// 弹簧阻尼插值（临界阻尼弹簧，常用于摄像机跟随）
    /// </summary>
    /// <param name="current">当前位置</param>
    /// <param name="target">目标位置</param>
    /// <param name="currentVelocity">当前速度（会被修改）</param>
    /// <param name="smoothTime">到达目标的大致时间</param>
    /// <param name="deltaTime">帧时间间隔</param>
    /// <param name="maxSpeed">最大速度限制</param>
    static Vector3 SmoothDamp(const Vector3& current, const Vector3& target, Vector3& currentVelocity,
                               float smoothTime, float deltaTime, float maxSpeed = 1e30f)
    {
        // 基于 Unity 的临界阻尼弹簧实现
        smoothTime = std::max(0.0001f, smoothTime);
        float omega = 2.0f / smoothTime;
        float x = omega * deltaTime;
        // 近似 exp(-x)
        float exp_factor = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);

        Vector3 diff = current - target;
        Vector3 origTarget = target;

        // 限制最大位移
        float maxDist = maxSpeed * smoothTime;
        diff = diff.ClampLength(maxDist);
        Vector3 clampedTarget = current - diff;

        Vector3 temp = (currentVelocity + diff * omega) * deltaTime;
        currentVelocity = (currentVelocity - temp * omega) * exp_factor;
        Vector3 result = clampedTarget + (diff + temp) * exp_factor;

        // 防止超调
        if (Dot(origTarget - current, result - origTarget) > 0.0f)
        {
            result = origTarget;
            currentVelocity = (result - origTarget) * (1.0f / deltaTime);
        }

        return result;
    }

    //  todo 反射和投影
    Vector3 Reflect(const Vector3& normal) const
    {
        // 注意：使用 normal * scalar 避免依赖全局左乘运算符
        return *this - normal * (2.0f * Dot(*this, normal));
    }

    Vector3 Project(const Vector3& onto) const
    {
        float dot = Dot(*this, onto);
        float ontoLenSq = onto.LengthSquared();
        return ontoLenSq > 0.0f ? onto * (dot / ontoLenSq) : Vector3::Zero;
    }
};

// 标量乘法（左操作数）
inline Vector3 operator*(float scalar, const Vector3& vec) { return vec * scalar; }

// ==================== Vector4 ====================
struct [[nodiscard]] Vector4
{
    float X, Y, Z, W;

    // 构造函数
    Vector4() : X(0.0f), Y(0.0f), Z(0.0f), W(0.0f) {}
    Vector4(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}
    explicit Vector4(float scalar) : X(scalar), Y(scalar), Z(scalar), W(scalar) {}
    Vector4(const Vector3& xyz, float w) : X(xyz.X), Y(xyz.Y), Z(xyz.Z), W(w) {}

    // 从 glm 构造/转换
    explicit Vector4(const glm::vec4& v) : X(v.x), Y(v.y), Z(v.z), W(v.w) {}
    operator glm::vec4() const { return {X, Y, Z, W}; }

    // 获取 XYZ 部分作为 Vector3
    Vector3 GetXYZ() const { return {X, Y, Z}; }

    // 常量
    static const Vector4 Zero;
    static const Vector4 One;

    // 运算符重载
    Vector4 operator+(const Vector4& other) const { return {X + other.X, Y + other.Y, Z + other.Z, W + other.W}; }
    Vector4 operator-(const Vector4& other) const { return {X - other.X, Y - other.Y, Z - other.Z, W - other.W}; }
    Vector4 operator*(const Vector4& other) const { return {X * other.X, Y * other.Y, Z * other.Z, W * other.W}; }
    Vector4 operator/(const Vector4& other) const { return {X / other.X, Y / other.Y, Z / other.Z, W / other.W}; }
    Vector4 operator*(float scalar) const { return {X * scalar, Y * scalar, Z * scalar, W * scalar}; }
    Vector4 operator/(float scalar) const { return {X / scalar, Y / scalar, Z / scalar, W / scalar}; }
    Vector4 operator-() const { return {-X, -Y, -Z, -W}; }

    Vector4& operator+=(const Vector4& other) { X += other.X; Y += other.Y; Z += other.Z; W += other.W; return *this; }
    Vector4& operator-=(const Vector4& other) { X -= other.X; Y -= other.Y; Z -= other.Z; W -= other.W; return *this; }
    Vector4& operator*=(const Vector4& other) { X *= other.X; Y *= other.Y; Z *= other.Z; W *= other.W; return *this; }
    Vector4& operator/=(const Vector4& other) { X /= other.X; Y /= other.Y; Z /= other.Z; W /= other.W; return *this; }
    Vector4& operator*=(float scalar) { X *= scalar; Y *= scalar; Z *= scalar; W *= scalar; return *this; }
    Vector4& operator/=(float scalar) { X /= scalar; Y /= scalar; Z /= scalar; W /= scalar; return *this; }

    bool operator==(const Vector4& other) const { return X == other.X && Y == other.Y && Z == other.Z && W == other.W; }
    bool operator!=(const Vector4& other) const { return !(*this == other); }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y + Z * Z + W * W); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y + Z * Z + W * W; }

    Vector4 Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Vector4{X / len, Y / len, Z / len, W / len} : Vector4::Zero;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; Z /= len; W /= len; }
    }

    static float Dot(const Vector4& a, const Vector4& b) { return a.X * b.X + a.Y * b.Y + a.Z * b.Z + a.W * b.W; }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// </summary>
    [[nodiscard]] bool Equals(const Vector4& other, float epsilon = 1e-6f) const
    {
        return std::abs(X - other.X) <= epsilon &&
               std::abs(Y - other.Y) <= epsilon &&
               std::abs(Z - other.Z) <= epsilon &&
               std::abs(W - other.W) <= epsilon;
    }

    /// <summary>
    /// 逐分量取最小值
    /// </summary>
    static Vector4 Min(const Vector4& a, const Vector4& b)
    {
        return {std::min(a.X, b.X), std::min(a.Y, b.Y), std::min(a.Z, b.Z), std::min(a.W, b.W)};
    }

    /// <summary>
    /// 逐分量取最大值
    /// </summary>
    static Vector4 Max(const Vector4& a, const Vector4& b)
    {
        return {std::max(a.X, b.X), std::max(a.Y, b.Y), std::max(a.Z, b.Z), std::max(a.W, b.W)};
    }

    // 线性插值
    static Vector4 Lerp(const Vector4& a, const Vector4& b, float t)
    {
        return a + (b - a) * t;
    }
};

// 标量乘法（左操作数）
inline Vector4 operator*(float scalar, const Vector4& vec) { return vec * scalar; }

// ==================== Matrix3 ====================
struct [[nodiscard]] Matrix3
{
    // 3x3 矩阵，按列主序存储（与 glm 一致）
    float M[3][3];

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

    // 从 glm 构造/转换
    explicit Matrix3(const glm::mat3& m)
    {
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                M[i][j] = m[i][j];
    }

    operator glm::mat3() const
    {
        glm::mat3 result;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                result[i][j] = M[i][j];
        return result;
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
    float M[4][4];

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

    // 从 glm 构造/转换
    explicit Matrix4(const glm::mat4& m)
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                M[i][j] = m[i][j];
    }

    operator glm::mat4() const
    {
        glm::mat4 result;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                result[i][j] = M[i][j];
        return result;
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
    Quat GetRotation() const;

    /// <summary>
    /// 提取左上角 3x3 旋转/缩放矩阵
    /// </summary>
    Matrix3 ToMatrix3() const;

    // 变换辅助函数
    static Matrix4 Translate(const Vector3& translation);
    static Matrix4 Rotate(float angleRadians, const Vector3& axis);
    static Matrix4 Scale(const Vector3& scale);
    static Matrix4 LookAt(const Vector3& eye, const Vector3& center, const Vector3& up);
    static Matrix4 Perspective(float fovRadians, float aspect, float nearPlane, float farPlane);

    /// OpenGL 专用透视投影（NDC 深度范围 [-1, 1]）
    /// 项目默认 GLM_FORCE_DEPTH_ZERO_TO_ONE 是 Vulkan 风格 [0,1]，
    /// macOS OpenGL 4.1 不支持 glClipControl，需要使用 [-1,1] 范围
    static Matrix4 PerspectiveGL(float fovRadians, float aspect, float nearPlane, float farPlane);

    static Matrix4 Orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);
};

// ==================== Quat（四元数） ====================
struct [[nodiscard]] Quat
{
    float X, Y, Z, W;  // W 是实部，X,Y,Z 是虚部

    // 构造函数
    Quat() : X(0.0f), Y(0.0f), Z(0.0f), W(1.0f) {}  // 单位四元数
    Quat(float x, float y, float z, float w) : X(x), Y(y), Z(z), W(w) {}

    // 从轴角构造
    Quat(const Vector3& axis, float angleRadians)
    {
        float halfAngle = angleRadians * 0.5f;
        float s = std::sin(halfAngle);
        Vector3 normalizedAxis = axis.Normalize();
        X = normalizedAxis.X * s;
        Y = normalizedAxis.Y * s;
        Z = normalizedAxis.Z * s;
        W = std::cos(halfAngle);
    }

    // 从欧拉角构造（yaw, pitch, roll）
    static Quat FromEuler(float yaw, float pitch, float roll);

    // 从 glm 构造/转换
    explicit Quat(const glm::quat& q) : X(q.x), Y(q.y), Z(q.z), W(q.w) {}
    operator glm::quat() const { return {W, X, Y, Z}; }

    // 常量
    static const Quat Identity;

    // 运算符重载
    Quat operator+(const Quat& other) const { return {X + other.X, Y + other.Y, Z + other.Z, W + other.W}; }
    Quat operator-(const Quat& other) const { return {X - other.X, Y - other.Y, Z - other.Z, W - other.W}; }
    Quat operator-() const { return {-X, -Y, -Z, -W}; }
    Quat operator*(float scalar) const { return {X * scalar, Y * scalar, Z * scalar, W * scalar}; }
    Quat operator/(float scalar) const { return {X / scalar, Y / scalar, Z / scalar, W / scalar}; }

    Quat& operator+=(const Quat& other) { X += other.X; Y += other.Y; Z += other.Z; W += other.W; return *this; }
    Quat& operator-=(const Quat& other) { X -= other.X; Y -= other.Y; Z -= other.Z; W -= other.W; return *this; }
    Quat& operator*=(float scalar) { X *= scalar; Y *= scalar; Z *= scalar; W *= scalar; return *this; }
    Quat& operator/=(float scalar) { X /= scalar; Y /= scalar; Z /= scalar; W /= scalar; return *this; }

    // 四元数乘法（表示旋转的组合）
    Quat operator*(const Quat& other) const
    {
        return {
            W * other.X + X * other.W + Y * other.Z - Z * other.Y,
            W * other.Y - X * other.Z + Y * other.W + Z * other.X,
            W * other.Z + X * other.Y - Y * other.X + Z * other.W,
            W * other.W - X * other.X - Y * other.Y - Z * other.Z
        };
    }

    Quat& operator*=(const Quat& other) { *this = *this * other; return *this; }

    // 旋转一个向量
    Vector3 operator*(const Vector3& vec) const
    {
        // q * v * q^-1
        Quat vecQuat(vec.X, vec.Y, vec.Z, 0.0f);
        Quat result = *this * vecQuat * Inverse();
        return {result.X, result.Y, result.Z};
    }

    bool operator==(const Quat& other) const { return X == other.X && Y == other.Y && Z == other.Z && W == other.W; }
    bool operator!=(const Quat& other) const { return !(*this == other); }

    /// <summary>
    /// 近似相等比较（考虑浮点误差）
    /// 注意：四元数 q 和 -q 表示相同旋转，此方法会同时检查两种情况
    /// </summary>
    [[nodiscard]] bool Equals(const Quat& other, float epsilon = 1e-6f) const
    {
        // q 和 -q 表示相同旋转
        bool direct = std::abs(X - other.X) <= epsilon &&
                       std::abs(Y - other.Y) <= epsilon &&
                       std::abs(Z - other.Z) <= epsilon &&
                       std::abs(W - other.W) <= epsilon;
        if (direct) return true;
        bool negated = std::abs(X + other.X) <= epsilon &&
                        std::abs(Y + other.Y) <= epsilon &&
                        std::abs(Z + other.Z) <= epsilon &&
                        std::abs(W + other.W) <= epsilon;
        return negated;
    }

    // 常用方法
    [[nodiscard]] float Length() const { return std::sqrt(X * X + Y * Y + Z * Z + W * W); }
    [[nodiscard]] float LengthSquared() const { return X * X + Y * Y + Z * Z + W * W; }

    Quat Normalize() const
    {
        float len = Length();
        return len > 0.0f ? Quat{X / len, Y / len, Z / len, W / len} : Quat::Identity;
    }

    void NormalizeInPlace()
    {
        float len = Length();
        if (len > 0.0f) { X /= len; Y /= len; Z /= len; W /= len; }
    }

    Quat Conjugate() const { return {-X, -Y, -Z, W}; }

    Quat Inverse() const
    {
        float lenSq = LengthSquared();
        return lenSq > 0.0f ? Conjugate() / lenSq : Quat::Identity;
    }

    // 转换为旋转矩阵
    Matrix4 ToMatrix4() const;
    Matrix3 ToMatrix3() const;

    // 转换为欧拉角（返回 yaw, pitch, roll）
    Vector3 ToEulerAngles() const;

    // 线性插值（非归一化，快速但不精确）
    static Quat Lerp(const Quat& a, const Quat& b, float t)
    {
        return (a * (1.0f - t) + b * t).Normalize();
    }

    // 球面线性插值（归一化，精确）
    static Quat Slerp(const Quat& a, const Quat& b, float t);

    // 获取旋转轴和角度
    void GetAxisAngle(Vector3& outAxis, float& outAngleRadians) const;
};

// 标量乘法（左操作数）
inline Quat operator*(float scalar, const Quat& q) { return q * scalar; }

} // namespace TE
