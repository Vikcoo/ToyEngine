// ToyEngine Scene Module
// TSceneComponent - 带 Transform 的组件
// 对应 UE5 的 USceneComponent
//
// 在 TComponent 基础上增加 Transform（位置/旋转/缩放）
// 提供 GetWorldMatrix() 获取世界变换矩阵

#pragma once

#include "Component.h"
#include "Math/Transform.h"

namespace TE {

/// 带 Transform 的组件
///
/// UE5 映射：
/// - USceneComponent: 有 Transform，可以被附加到场景层级中
/// - 提供 GetComponentToWorld() 返回世界变换
///
/// ToyEngine 简化版：直接持有 Transform，GetWorldMatrix() 返回 TRS 矩阵
class TSceneComponent : public TComponent
{
public:
    TSceneComponent() = default;
    ~TSceneComponent() override = default;

    /// 获取本地 Transform（相对于 Actor 根组件）
    [[nodiscard]] Transform& GetTransform() { return m_Transform; }
    [[nodiscard]] const Transform& GetTransform() const { return m_Transform; }
    void SetTransform(const Transform& transform) { m_Transform = transform; }

    /// 获取世界变换矩阵
    /// 当前简化版：直接返回 m_Transform.ToMatrix()
    /// 完整版应考虑组件层级（parent → child）
    [[nodiscard]] Matrix4 GetWorldMatrix() const;

    /// 位置快捷访问
    void SetPosition(const Vector3& pos) { m_Transform.Position = pos; }
    [[nodiscard]] const Vector3& GetPosition() const { return m_Transform.Position; }

    /// 旋转快捷访问
    void SetRotation(const Quat& rot) { m_Transform.Rotation = rot; }
    [[nodiscard]] const Quat& GetRotation() const { return m_Transform.Rotation; }

    /// 缩放快捷访问
    void SetScale(const Vector3& scale) { m_Transform.Scale = scale; }
    [[nodiscard]] const Vector3& GetScale() const { return m_Transform.Scale; }

protected:
    Transform m_Transform;
};

} // namespace TE
