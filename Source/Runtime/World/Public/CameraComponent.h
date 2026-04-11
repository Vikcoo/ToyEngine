// ToyEngine Scene Module
// TCameraComponent - 相机组件
// 对应 UE5 的 UCameraComponent
//
// 持有相机参数（FOV、近远裁剪面等），
// 提供 BuildViewInfo() 构建 FViewInfo 传递给渲染器

#pragma once

#include "SceneComponent.h"
#include "SceneViewInfo.h"

namespace TE {

/// 相机组件
///
/// UE5 映射：
/// - UCameraComponent: 定义相机参数
/// - GetViewInfo(): 构建 FMinimalViewInfo
///
/// ToyEngine 简化版：
/// - 持有 FOV、AspectRatio、NearPlane、FarPlane
/// - BuildViewInfo() 使用自身 Transform 构建 View 矩阵，
///   结合投影参数构建 Projection 矩阵，返回 FViewInfo
class TCameraComponent : public TSceneComponent
{
public:
    TCameraComponent() = default;
    ~TCameraComponent() override = default;

    /// 相机专用 LookAt（相机局部前向约定为 -Z）
    void LookAt(const Vector3& target, const Vector3& worldUp = Vector3::Up);

    /// 构建视图信息（每帧调用）
    /// 使用当前 Transform 的位置和朝向构建 View 矩阵
    /// 使用 FOV、Aspect、Near/Far 构建 Projection 矩阵
    [[nodiscard]] FViewInfo BuildViewInfo() const;

    // 相机参数访问
    void SetFOV(float fovDegrees) { m_FOVDegrees = fovDegrees; }
    [[nodiscard]] float GetFOV() const { return m_FOVDegrees; }

    void SetAspectRatio(float aspect) { m_AspectRatio = aspect; }
    [[nodiscard]] float GetAspectRatio() const { return m_AspectRatio; }

    void SetNearPlane(float nearPlane) { m_NearPlane = nearPlane; }
    [[nodiscard]] float GetNearPlane() const { return m_NearPlane; }

    void SetFarPlane(float farPlane) { m_FarPlane = farPlane; }
    [[nodiscard]] float GetFarPlane() const { return m_FarPlane; }

    void SetViewportSize(float width, float height)
    {
        m_ViewportWidth = width;
        m_ViewportHeight = height;
        if (height > 0.0f)
            m_AspectRatio = width / height;
    }

private:
    float m_FOVDegrees = 60.0f;         // 垂直视场角（度）
    float m_AspectRatio = 16.0f / 9.0f; // 宽高比
    float m_NearPlane = 0.1f;           // 近裁剪面
    float m_FarPlane = 100.0f;          // 远裁剪面
    float m_ViewportWidth = 1280.0f;
    float m_ViewportHeight = 720.0f;
};

} // namespace TE
