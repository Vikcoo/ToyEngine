// ToyEngine Renderer Module
// FPrimitiveSceneProxy - 可渲染组件的渲染侧镜像
// 对应 UE5 的 FPrimitiveSceneProxy
//
// 核心设计理念（UE5 Game/Render 数据分离）：
// - 游戏侧有 TPrimitiveComponent，持有逻辑数据（Transform、网格引用等）
// - 渲染侧有 FPrimitiveSceneProxy，持有实例级渲染快照（Transform、可见性、资源引用）
// - GPU 资源由独立 RenderData/Renderer 层管理，SceneProxy 仅持有引用
// - 两者是独立的对象，通过 SyncToScene() 同步数据
// - 单线程版本中直接赋值同步；将来双线程改为命令队列异步同步

#pragma once

#include "Math/MathTypes.h"
#include "FMeshDrawCommand.h"
#include <vector>

namespace TE {

/// 可渲染组件的渲染侧镜像基类
///
/// UE5 映射：
/// - FPrimitiveSceneProxy: 所有可渲染 Proxy 的基类
/// - 子类如 FStaticMeshSceneProxy、FSkeletalMeshSceneProxy 等
///
/// 生命周期：
/// 1. TPrimitiveComponent::RegisterToScene() 时创建
/// 2. SyncToScene() 时更新 WorldMatrix
/// 3. TPrimitiveComponent::UnregisterFromScene() 时销毁
class FPrimitiveSceneProxy
{
public:
    virtual ~FPrimitiveSceneProxy() = default;

    /// 设置世界变换矩阵（SyncToScene 时从 Component 同步过来）
    /// 单线程：直接赋值
    /// 将来双线程：通过渲染命令队列传递
    void SetWorldMatrix(const Matrix4& matrix) { m_WorldMatrix = matrix; }
    [[nodiscard]] const Matrix4& GetWorldMatrix() const { return m_WorldMatrix; }

    /// 收集绘制命令（SceneRenderer 调用）
    /// 子类 override 此方法，填充 FMeshDrawCommand 列表
    /// 支持多 Section 的 Proxy 返回多条绘制命令
    /// @param outCommands 输出的绘制命令列表
    virtual void GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const = 0;

protected:
    FPrimitiveSceneProxy() = default;

    Matrix4 m_WorldMatrix;  // 世界变换矩阵（Model 矩阵）
};

} // namespace TE
