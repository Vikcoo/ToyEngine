// ToyEngine Scene Module
// TPrimitiveComponent - 可渲染组件
// 对应 UE5 的 UPrimitiveComponent
//
// UE5 核心同步类：
// - 游戏侧：持有逻辑数据（Transform、碰撞等）
// - 渲染侧：通过 IRenderScene 创建/更新/销毁渲染镜像
// - 同步：MarkRenderStateDirty() 标记脏，SyncToScene() 同步到渲染场景接口

#pragma once

#include "RenderScene.h"
#include "SceneComponent.h"

namespace TE {

/// 可渲染组件
///
/// UE5 映射：
/// - UPrimitiveComponent: 所有可渲染/可碰撞组件的基类
/// - 核心职责：构建渲染创建信息并通过渲染场景接口创建渲染镜像
/// - MarkRenderStateDirty() 通知渲染侧数据已变化
///
/// 在 ToyEngine 中：
/// - RegisterToRenderScene() 时通过 IRenderScene 创建渲染对象
/// - MarkRenderStateDirty() 设置脏标记
/// - World::SyncToScene() 遍历脏 Component，将 WorldMatrix 同步到渲染场景接口
class TPrimitiveComponent : public TSceneComponent
{
public:
    TPrimitiveComponent() = default;
    ~TPrimitiveComponent() override;

    /// 构建渲染创建信息（子类 override）
    [[nodiscard]] virtual bool BuildRenderCreateInfo(RenderPrimitiveCreateInfo& outCreateInfo) const { return false; }

    /// 标记渲染状态脏
    /// 游戏逻辑修改 Transform 后调用此方法
    /// 下一次 SyncToScene() 时会将最新数据同步到渲染场景接口
    void MarkRenderStateDirty() { m_RenderStateDirty = true; }

    /// 注册到渲染场景（通过渲染场景接口创建渲染对象）
    void RegisterToRenderScene(IRenderScene* renderScene);

    /// 从渲染场景注销（通过渲染场景接口销毁渲染对象）
    void UnregisterFromRenderScene(IRenderScene* renderScene);

    /// 获取渲染对象句柄
    [[nodiscard]] RenderPrimitiveHandle GetRenderPrimitiveHandle() const { return m_RenderPrimitiveHandle; }
    [[nodiscard]] bool IsRegisteredToRenderScene() const { return m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle; }

    /// 脏标记查询/清除
    [[nodiscard]] bool IsRenderStateDirty() const { return m_RenderStateDirty; }
    void ClearRenderStateDirty() { m_RenderStateDirty = false; }

protected:
    IRenderScene* m_BoundRenderScene = nullptr;
    RenderPrimitiveHandle m_RenderPrimitiveHandle = InvalidRenderPrimitiveHandle;
    bool m_RenderStateDirty = true;  // 初始化时默认脏
};

} // namespace TE
