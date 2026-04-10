// ToyEngine Scene Module
// TPrimitiveComponent - 可渲染组件
// 对应 UE5 的 UPrimitiveComponent
//
// UE5 核心桥接类：
// - 游戏侧：持有逻辑数据（Transform、碰撞等）
// - 渲染侧：通过 IRenderSceneBridge 创建/更新/销毁渲染镜像
// - 桥接：MarkRenderStateDirty() 标记脏，SyncToScene() 同步到渲染桥接层

#pragma once

#include "RenderSceneBridge.h"
#include "SceneComponent.h"

namespace TE {

/// 可渲染组件
///
/// UE5 映射：
/// - UPrimitiveComponent: 所有可渲染/可碰撞组件的基类
/// - 核心职责：构建渲染创建信息并通过桥接层创建渲染镜像
/// - MarkRenderStateDirty() 通知渲染侧数据已变化
///
/// 在 ToyEngine 中：
/// - RegisterToRenderScene() 时通过 IRenderSceneBridge 创建渲染对象
/// - MarkRenderStateDirty() 设置脏标记
/// - World::SyncToScene() 遍历脏 Component，将 WorldMatrix 同步到桥接层
class TPrimitiveComponent : public TSceneComponent
{
public:
    TPrimitiveComponent() = default;
    ~TPrimitiveComponent() override;

    /// 构建渲染创建信息（子类 override）
    [[nodiscard]] virtual bool BuildRenderCreateInfo(RenderPrimitiveCreateInfo& outCreateInfo) const { return false; }

    /// 标记渲染状态脏
    /// 游戏逻辑修改 Transform 后调用此方法
    /// 下一次 SyncToScene() 时会将最新数据同步到渲染桥接层
    void MarkRenderStateDirty() { m_RenderStateDirty = true; }

    /// 注册到渲染场景（通过桥接层创建渲染对象）
    void RegisterToRenderScene(IRenderSceneBridge* renderBridge);

    /// 从渲染场景注销（通过桥接层销毁渲染对象）
    void UnregisterFromRenderScene(IRenderSceneBridge* renderBridge);

    /// 获取渲染对象句柄
    [[nodiscard]] RenderPrimitiveHandle GetRenderPrimitiveHandle() const { return m_RenderPrimitiveHandle; }
    [[nodiscard]] bool IsRegisteredToRenderScene() const { return m_RenderPrimitiveHandle != InvalidRenderPrimitiveHandle; }

    /// 脏标记查询/清除
    [[nodiscard]] bool IsRenderStateDirty() const { return m_RenderStateDirty; }
    void ClearRenderStateDirty() { m_RenderStateDirty = false; }

protected:
    IRenderSceneBridge* m_BoundRenderBridge = nullptr;
    RenderPrimitiveHandle m_RenderPrimitiveHandle = InvalidRenderPrimitiveHandle;
    bool m_RenderStateDirty = true;  // 初始化时默认脏
};

} // namespace TE
