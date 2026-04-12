// ToyEngine Scene Module
// TCameraComponent 实现
// 构建 FViewInfo（View + Projection 矩阵）

#include "CameraComponent.h"
#include "Math/ScalarMath.h"
#include "Log/Log.h"

namespace TE {

void CameraComponent::LookAt(const Vector3& target, const Vector3& worldUp)
{
    const Vector3& eye = m_Transform.Position;
    if ((target - eye).LengthSquared() <= 1e-8f)
    {
        return;
    }

    // 相机约定：局部 -Z 为“看向前方”，因此直接从视图矩阵反解世界旋转。
    const Matrix4 view = Matrix4::LookAt(eye, target, worldUp);
    const Transform worldTransform = Transform::FromMatrix(view.Inverse());
    m_Transform.Rotation = worldTransform.Rotation.Normalize();
}

FViewInfo CameraComponent::BuildViewInfo() const
{
    FViewInfo viewInfo;

    // 1. 构建 View 矩阵
    // 从 Transform 获取相机位置和朝向
    const Vector3& eye = m_Transform.Position;
    const Vector3 forward = (-m_Transform.GetForward()).Normalize();
    const Vector3 up = m_Transform.GetUp();
    const Vector3 target = eye + forward;

    viewInfo.ViewMatrix = Matrix4::LookAt(eye, target, up);

    // 2. 构建 Projection 矩阵
    // 使用 PerspectiveGL：OpenGL [-1, 1] 深度范围
    const float fovRadians = Math::DegToRad(m_FOVDegrees);
    viewInfo.ProjectionMatrix = Matrix4::PerspectiveGL(fovRadians, m_AspectRatio, m_NearPlane, m_FarPlane);

    // 3. 预计算 ViewProjection
    viewInfo.UpdateViewProjectionMatrix();

    // 4. 视口尺寸
    viewInfo.ViewportWidth = m_ViewportWidth;
    viewInfo.ViewportHeight = m_ViewportHeight;

    return viewInfo;
}

} // namespace TE
