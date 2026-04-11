// ToyEngine Renderer Module
// FScene - 渲染场景
// 对应 UE5 的 FScene
//
// 渲染侧的场景容器，存储所有 SceneProxy 和 ViewInfo。
// 独立于 TWorld（游戏侧场景），渲染器只需要认 FScene，不需要知道 World 的存在。
// 这种分离保证了将来双线程改造时，渲染线程只需访问 FScene。

#pragma once

#include "FPrimitiveSceneProxy.h"
#include "FStaticMeshRenderData.h"
#include "FViewInfo.h"
#include "RenderScene.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace TE {

class RHIDevice;
class RHIPipeline;
class RHIShader;
class TStaticMesh;

/// 渲染场景
///
/// UE5 映射：
/// - FScene: 存储所有 Primitive 的 SceneProxy 列表
/// - 是 SceneRenderer 的数据来源
class FScene : public IRenderScene
{
public:
    explicit FScene(RHIDevice* device);
    ~FScene();

    /// 创建 Primitive Proxy 并添加到渲染场景，返回稳定句柄
    [[nodiscard]] RenderPrimitiveHandle CreatePrimitive(const RenderPrimitiveCreateInfo& createInfo) override;

    /// 通过句柄移除 Primitive Proxy
    void DestroyPrimitive(RenderPrimitiveHandle handle) override;

    /// 通过句柄更新 Primitive 世界矩阵
    void UpdatePrimitiveTransform(RenderPrimitiveHandle handle, const Matrix4& worldMatrix) override;

    /// 获取所有 Primitive Proxy（SceneRenderer 遍历用）
    [[nodiscard]] const std::vector<FPrimitiveSceneProxy*>& GetPrimitives() const { return m_Primitives; }

    /// 设置/获取视图信息
    void SetViewInfo(const FViewInfo& viewInfo) { m_ViewInfo = viewInfo; }
    [[nodiscard]] const FViewInfo& GetViewInfo() const { return m_ViewInfo; }

private:
    [[nodiscard]] bool EnsureStaticMeshPipeline();
    [[nodiscard]] std::shared_ptr<const FStaticMeshRenderData> GetOrCreateStaticMeshRenderData(const std::shared_ptr<TStaticMesh>& staticMesh);
    [[nodiscard]] RenderPrimitiveHandle InsertPrimitive(std::unique_ptr<FPrimitiveSceneProxy> proxy);
    void RebuildPrimitiveView();

    RHIDevice* m_Device = nullptr;
    RenderPrimitiveHandle m_NextHandle = InvalidRenderPrimitiveHandle + 1;
    std::unordered_map<RenderPrimitiveHandle, std::unique_ptr<FPrimitiveSceneProxy>> m_PrimitiveStorage;
    std::vector<FPrimitiveSceneProxy*> m_Primitives; // SceneRenderer 遍历视图
    std::unordered_map<const TStaticMesh*, std::weak_ptr<const FStaticMeshRenderData>> m_StaticMeshRenderDataCache;
    std::unique_ptr<RHIShader> m_StaticMeshVertexShader;
    std::unique_ptr<RHIShader> m_StaticMeshFragmentShader;
    std::unique_ptr<RHIPipeline> m_StaticMeshPipeline;
    FViewInfo m_ViewInfo; // 当前帧的视图信息
};

} // namespace TE
