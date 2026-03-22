// ToyEngine Scene Module
// TCameraComponent 实现
// 构建 FViewInfo（View + Projection 矩阵）

#include "CameraComponent.h"
#include "Math/MathUtils.h"
#include "Log/Log.h"

namespace TE {

FViewInfo TCameraComponent::BuildViewInfo() const
{
    FViewInfo viewInfo;

    // 1. 构建 View 矩阵
    // 从 Transform 获取相机位置和朝向
    const Vector3& eye = m_Transform.Position;
    Vector3 forward = m_Transform.GetForward();
    Vector3 up = m_Transform.GetUp();
    Vector3 target = eye + forward;

    viewInfo.ViewMatrix = Matrix4::LookAt(eye, target, up);

    // 2. 构建 Projection 矩阵
    // 使用 PerspectiveGL：OpenGL [-1, 1] 深度范围
    float fovRadians = Math::DegToRad(m_FOVDegrees);
    viewInfo.ProjectionMatrix = Matrix4::PerspectiveGL(fovRadians, m_AspectRatio, m_NearPlane, m_FarPlane);

    // 3. 预计算 ViewProjection
    viewInfo.UpdateViewProjectionMatrix();

    // 4. 视口尺寸
    viewInfo.ViewportWidth = m_ViewportWidth;
    viewInfo.ViewportHeight = m_ViewportHeight;

    return viewInfo;
}

} // namespace TE
