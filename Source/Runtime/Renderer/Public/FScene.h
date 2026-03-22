// ToyEngine Renderer Module
// FScene - 渲染场景
// 对应 UE5 的 FScene
//
// 渲染侧的场景容器，存储所有 SceneProxy 和 ViewInfo。
// 独立于 TWorld（游戏侧场景），渲染器只需要认 FScene，不需要知道 World 的存在。
// 这种分离保证了将来双线程改造时，渲染线程只需访问 FScene。

#pragma once

#include "FViewInfo.h"
#include <vector>

namespace TE {

class FPrimitiveSceneProxy;

/// 渲染场景
///
/// UE5 映射：
/// - FScene: 存储所有 Primitive 的 SceneProxy 列表
/// - 是 SceneRenderer 的数据来源
///
/// 注意：FScene 不拥有 Proxy 的内存（raw pointer），
/// Proxy 的生命周期由 TPrimitiveComponent 管理
class FScene
{
public:
    FScene() = default;
    ~FScene() = default;

    /// 添加 Primitive Proxy 到渲染场景
    /// 在 TPrimitiveComponent::RegisterToScene() 时调用
    void AddPrimitive(FPrimitiveSceneProxy* proxy);

    /// 从渲染场景移除 Primitive Proxy
    /// 在 TPrimitiveComponent::UnregisterFromScene() 时调用
    void RemovePrimitive(FPrimitiveSceneProxy* proxy);

    /// 获取所有 Primitive Proxy（SceneRenderer 遍历用）
    [[nodiscard]] const std::vector<FPrimitiveSceneProxy*>& GetPrimitives() const { return m_Primitives; }

    /// 设置/获取视图信息
    void SetViewInfo(const FViewInfo& viewInfo) { m_ViewInfo = viewInfo; }
    [[nodiscard]] const FViewInfo& GetViewInfo() const { return m_ViewInfo; }

private:
    std::vector<FPrimitiveSceneProxy*>  m_Primitives;   // 所有渲染对象的 Proxy
    FViewInfo                           m_ViewInfo;     // 当前帧的视图信息
};

} // namespace TE
