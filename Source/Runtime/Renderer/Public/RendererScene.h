// ToyEngine Renderer Module
// FScene - 渲染场景
// 对应 UE5 的 FScene
//
// 渲染侧的场景容器，存储所有 SceneProxy 和 ViewInfo。
// 独立于 TWorld（游戏侧场景），渲染器只需要认 FScene，不需要知道 World 的存在。
// 这种分离保证了将来双线程改造时，渲染线程只需访问 FScene。

#pragma once

#include "PrimitiveSceneInfo.h"
#include "PrimitiveSceneProxy.h"
#include "MeshDrawCommand.h"
#include "ViewInfo.h"
#include "RenderScene.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace TE {

class RHIDevice;
class RHIPipeline;
class PrimitiveComponent;
class FRenderResourceManager;

/// 渲染场景
///
/// UE5 映射：
/// - FScene: 存储所有 Primitive 的 SceneProxy 列表
/// - 是 SceneRenderer 的数据来源
class FScene : public IRenderScene
{
public:
    explicit FScene(RHIDevice* device);
    ~FScene() override;

    /// 注册由游戏侧组件创建好的 Primitive Proxy
    [[nodiscard]] bool AddPrimitive(const PrimitiveComponent* primitiveComponent,
                                    FPrimitiveComponentId primitiveComponentId,
                                    std::unique_ptr<FPrimitiveSceneProxy> proxy) override;

    /// 通过组件指针移除 Primitive Proxy
    void RemovePrimitive(FPrimitiveComponentId primitiveComponentId) override;

    /// 通过组件指针更新 Primitive 世界矩阵
    void UpdatePrimitiveTransform(FPrimitiveComponentId primitiveComponentId, const Matrix4& worldMatrix) override;

    /// 获取所有 Primitive Proxy（SceneRenderer 遍历用）
    [[nodiscard]] const std::vector<FPrimitiveSceneProxy*>& GetPrimitives() const { return m_Primitives; }
    [[nodiscard]] RHIPipeline* ResolvePreparedPipeline(EMeshPipelineKey pipelineKey) const;

    /// 设置/获取视图信息
    void SetViewInfo(const FViewInfo& viewInfo) { m_ViewInfo = viewInfo; }
    [[nodiscard]] const FViewInfo& GetViewInfo() const { return m_ViewInfo; }

private:
    [[nodiscard]] bool PrepareProxyResources(FPrimitiveSceneProxy& proxy);
    [[nodiscard]] bool InsertPrimitive(FPrimitiveComponentId primitiveComponentId,
                                       const PrimitiveComponent* primitiveComponent,
                                       std::unique_ptr<FPrimitiveSceneProxy> proxy);
    void RebuildPrimitiveView();

    std::unique_ptr<FRenderResourceManager> m_RenderResourceManager;
    std::unordered_map<FPrimitiveComponentId, std::unique_ptr<FPrimitiveSceneInfo>, FPrimitiveComponentIdHash> m_PrimitiveStorage;
    std::vector<FPrimitiveSceneProxy*> m_Primitives; // SceneRenderer 遍历视图
    FViewInfo m_ViewInfo; // 当前帧的视图信息
};

} // namespace TE
