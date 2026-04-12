// ToyEngine Scene Module
// TSceneComponent 实现

#include "SceneComponent.h"

namespace TE {

Matrix4 SceneComponent::GetWorldMatrix() const
{
    // 当前简化版：直接返回自身 Transform 的矩阵
    // 完整版应该考虑组件层级（递归乘以 parent 的 WorldMatrix）
    // 以及 Actor 的 RootComponent Transform
    return m_Transform.ToMatrix();
}

} // namespace TE
