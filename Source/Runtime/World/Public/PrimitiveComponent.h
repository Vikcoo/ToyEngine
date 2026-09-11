// ToyEngine Scene Module
// TPrimitiveComponent - 可渲染组件
// 对应 UE5 的 UPrimitiveComponent
//
// UE5 核心同步类：
// - 游戏侧：持有逻辑数据（Transform、碰撞等）
// - 渲染侧：通过命令收集器创建/更新/销毁渲染镜像
// - 同步：MarkRenderStateDirty() 标记脏，SyncToScene() 记录渲染场景命令

#pragma once

#include "PrimitiveSceneProxy.h"
#include "PrimitiveComponentId.h"
#include "SceneComponent.h"

#include <memory>

namespace TE {

class FRenderSceneCommandRecorder;

/// 可渲染组件
///
/// UE5 映射：
/// - UPrimitiveComponent: 所有可渲染/可碰撞组件的基类
/// - 核心职责：通过 CreateSceneProxy 语义生成渲染镜像
/// - MarkRenderStateDirty() 通知渲染侧数据已变化
///
/// 在 ToyEngine 中：
/// - World 注册组件时调用 CreateRenderState() 创建具体 SceneProxy
/// - MarkRenderStateDirty() 设置脏标记
/// - World::SyncToScene() 遍历脏 Component，将 WorldMatrix 记录为渲染场景命令
class PrimitiveComponent : public SceneComponent
{
public:
    PrimitiveComponent();
    ~PrimitiveComponent() override = default;

    /// CreateSceneProxy 语义：子类直接创建具体渲染代理
    [[nodiscard]] virtual std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy() const { return nullptr; }

    /// 标记渲染状态脏
    /// 游戏逻辑修改 Transform 后调用此方法
    /// 下一次 SyncToScene() 时会记录最新状态
    void MarkRenderStateDirty() { m_RenderStateDirty = true; }

    /// 是否已为当前 World 注册创建渲染状态
    [[nodiscard]] bool IsRenderStateCreated() const { return m_IsRenderStateCreated; }
    [[nodiscard]] FPrimitiveComponentId GetPrimitiveComponentId() const { return m_PrimitiveComponentId; }

    /// 脏标记查询/清除
    [[nodiscard]] bool IsRenderStateDirty() const { return m_RenderStateDirty; }
    void ClearRenderStateDirty() { m_RenderStateDirty = false; }

protected:
    FPrimitiveComponentId m_PrimitiveComponentId;
    bool m_IsRenderStateCreated = false;
    bool m_RenderStateDirty = true;  // 初始化时默认脏

private:
    friend class World;

    /** 为已注册到 World 的组件创建渲染状态。 */
    void CreateRenderState(FRenderSceneCommandRecorder& recorder);
    /** 销毁渲染状态，但不改变 Actor 对组件的所有权。 */
    void DestroyRenderState(FRenderSceneCommandRecorder& recorder);
};

} // namespace TE
