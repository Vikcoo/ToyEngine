// ToyEngine Scene Module
// TPrimitiveComponent - 可渲染组件
// 对应 UE5 的 UPrimitiveComponent
//
// UE5 核心桥接类：
// - 游戏侧：持有逻辑数据（Transform、碰撞等）
// - 渲染侧：通过 CreateSceneProxy() 创建 FPrimitiveSceneProxy
// - 桥接：MarkRenderStateDirty() 标记脏，SyncToScene() 同步到 Proxy

#pragma once

#include "SceneComponent.h"

namespace TE {

class FScene;
class FPrimitiveSceneProxy;
class RHIDevice;

/// 可渲染组件
///
/// UE5 映射：
/// - UPrimitiveComponent: 所有可渲染/可碰撞组件的基类
/// - 核心职责：CreateSceneProxy() 创建渲染镜像
/// - MarkRenderStateDirty() 通知渲染侧数据已变化
///
/// 在 ToyEngine 中：
/// - RegisterToScene() 时调用 CreateSceneProxy() 创建 Proxy，加入 FScene
/// - MarkRenderStateDirty() 设置脏标记
/// - World::SyncToScene() 遍历脏 Component，将 WorldMatrix 同步到 Proxy
class TPrimitiveComponent : public TSceneComponent
{
public:
    TPrimitiveComponent() = default;
    ~TPrimitiveComponent() override;

    /// 创建渲染侧镜像（子类 override）
    /// UE5 核心虚方法：每种可渲染组件返回不同类型的 SceneProxy
    /// @param device RHI 设备（用于在 Proxy 构造函数中创建 GPU 资源）
    /// @return 新创建的 SceneProxy 指针（所有权归 Component 管理）
    virtual FPrimitiveSceneProxy* CreateSceneProxy(RHIDevice* device) { return nullptr; }

    /// 标记渲染状态脏
    /// 游戏逻辑修改 Transform 后调用此方法
    /// 下一次 SyncToScene() 时会将最新数据同步到 Proxy
    void MarkRenderStateDirty() { m_RenderStateDirty = true; }

    /// 注册到渲染场景（创建 Proxy 并加入 FScene）
    void RegisterToScene(FScene* scene, RHIDevice* device);

    /// 从渲染场景注销（从 FScene 移除 Proxy 并销毁）
    void UnregisterFromScene(FScene* scene);

    /// 获取渲染侧 Proxy
    [[nodiscard]] FPrimitiveSceneProxy* GetSceneProxy() const { return m_SceneProxy; }

    /// 脏标记查询/清除
    [[nodiscard]] bool IsRenderStateDirty() const { return m_RenderStateDirty; }
    void ClearRenderStateDirty() { m_RenderStateDirty = false; }

protected:
    FPrimitiveSceneProxy*   m_SceneProxy = nullptr;
    bool                    m_RenderStateDirty = true;  // 初始化时默认脏
};

} // namespace TE
