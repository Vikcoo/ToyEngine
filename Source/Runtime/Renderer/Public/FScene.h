// ToyEngine Renderer Module
// FScene - 渲染场景
// 对应 UE5 的 FScene
//
// 渲染侧的场景容器，存储所有 SceneProxy 和 ViewInfo。
// 独立于 TWorld（游戏侧场景），渲染器只需要认 FScene，不需要知道 World 的存在。
// 这种分离保证了将来双线程改造时，渲染线程只需访问 FScene。

#pragma once

#include "FPrimitiveSceneProxy.h"
#include "FViewInfo.h"
#include "RenderSceneBridge.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace TE {

/// 渲染场景
///
/// UE5 映射：
/// - FScene: 存储所有 Primitive 的 SceneProxy 列表
/// - 是 SceneRenderer 的数据来源
class FScene
{
public:
    FScene() = default;
    ~FScene();

    /// 添加 Primitive Proxy 到渲染场景，返回稳定句柄
    [[nodiscard]] RenderPrimitiveHandle AddPrimitive(std::unique_ptr<FPrimitiveSceneProxy> proxy);

    /// 通过句柄移除 Primitive Proxy
    void RemovePrimitive(RenderPrimitiveHandle handle);

    /// 通过句柄更新 Primitive 世界矩阵
    void UpdatePrimitiveWorldMatrix(RenderPrimitiveHandle handle, const Matrix4& worldMatrix);

    /// 获取所有 Primitive Proxy（SceneRenderer 遍历用）
    [[nodiscard]] const std::vector<FPrimitiveSceneProxy*>& GetPrimitives() const { return m_Primitives; }

    /// 设置/获取视图信息
    void SetViewInfo(const FViewInfo& viewInfo) { m_ViewInfo = viewInfo; }
    [[nodiscard]] const FViewInfo& GetViewInfo() const { return m_ViewInfo; }

private:
    void RebuildPrimitiveView();

    RenderPrimitiveHandle m_NextHandle = InvalidRenderPrimitiveHandle + 1;
    std::unordered_map<RenderPrimitiveHandle, std::unique_ptr<FPrimitiveSceneProxy>> m_PrimitiveStorage;
    std::vector<FPrimitiveSceneProxy*> m_Primitives; // SceneRenderer 遍历视图
    FViewInfo m_ViewInfo; // 当前帧的视图信息
};

} // namespace TE
