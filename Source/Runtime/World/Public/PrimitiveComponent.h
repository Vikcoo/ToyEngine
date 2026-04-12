// ToyEngine Scene Module
// TPrimitiveComponent - 可渲染组件
// 对应 UE5 的 UPrimitiveComponent
//
// UE5 核心同步类：
// - 游戏侧：持有逻辑数据（Transform、碰撞等）
// - 渲染侧：通过 IRenderScene 创建/更新/销毁渲染镜像
// - 同步：MarkRenderStateDirty() 标记脏，SyncToScene() 同步到渲染场景接口

#pragma once

#include "PrimitiveSceneProxy.h"
#include "PrimitiveComponentId.h"
#include "RenderScene.h"
#include "SceneComponent.h"

namespace TE {

/// 可渲染组件
///
/// UE5 映射：
/// - UPrimitiveComponent: 所有可渲染/可碰撞组件的基类
/// - 核心职责：通过 CreateSceneProxy 语义生成渲染镜像
/// - MarkRenderStateDirty() 通知渲染侧数据已变化
///
/// 在 ToyEngine 中：
/// - RegisterToRenderScene() 时由组件创建具体 SceneProxy，再交给 IRenderScene 注册
/// - MarkRenderStateDirty() 设置脏标记
/// - World::SyncToScene() 遍历脏 Component，将 WorldMatrix 同步到渲染场景接口
class PrimitiveComponent : public SceneComponent
{
public:
    PrimitiveComponent();
    ~PrimitiveComponent() override;

    /// CreateSceneProxy 语义：子类直接创建具体渲染代理
    [[nodiscard]] virtual std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const { return nullptr; }

    /// 标记渲染状态脏
    /// 游戏逻辑修改 Transform 后调用此方法
    /// 下一次 SyncToScene() 时会将最新数据同步到渲染场景接口
    void MarkRenderStateDirty() { m_RenderStateDirty = true; }

    /// 注册到渲染场景（通过渲染场景接口创建渲染对象）
    void RegisterToRenderScene(IRenderScene* renderScene);

    /// 从渲染场景注销（通过渲染场景接口销毁渲染对象）
    void UnregisterFromRenderScene(IRenderScene* renderScene);

    /// 是否已注册到渲染场景
    [[nodiscard]] bool IsRegisteredToRenderScene() const { return m_IsRegisteredToRenderScene; }
    [[nodiscard]] FPrimitiveComponentId GetPrimitiveComponentId() const { return m_PrimitiveComponentId; }

    /// 脏标记查询/清除
    [[nodiscard]] bool IsRenderStateDirty() const { return m_RenderStateDirty; }
    void ClearRenderStateDirty() { m_RenderStateDirty = false; }

protected:
    IRenderScene* m_BoundRenderScene = nullptr;
    FPrimitiveComponentId m_PrimitiveComponentId;
    bool m_IsRegisteredToRenderScene = false;
    bool m_RenderStateDirty = true;  // 初始化时默认脏
};

} // namespace TE
