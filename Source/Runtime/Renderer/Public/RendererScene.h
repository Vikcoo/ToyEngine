// ToyEngine Renderer Module
// FScene - 渲染侧场景容器

#pragma once

#include "LightComponentId.h"
#include "LightSceneProxy.h"
#include "PrimitiveComponentId.h"
#include "PrimitiveSceneInfo.h"
#include "PrimitiveSceneProxy.h"
#include "MeshDrawCommand.h"
#include "RenderScene.h"
#include "ViewInfo.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace TE {

class FRenderResourceManager;
class LightComponent;
class PrimitiveComponent;
class RHIDevice;
class RHIPipeline;
class RHISampler;
class RHITexture;
class StaticMesh;

class FScene : public IRenderScene
{
public:
    explicit FScene(RHIDevice* device);
    ~FScene() override;

    [[nodiscard]] bool AddPrimitive(const PrimitiveComponent* primitiveComponent,
                                    FPrimitiveComponentId primitiveComponentId,
                                    std::unique_ptr<FPrimitiveSceneProxy> proxy) override;
    void RemovePrimitive(FPrimitiveComponentId primitiveComponentId) override;
    void UpdatePrimitiveTransform(FPrimitiveComponentId primitiveComponentId, const Matrix4& worldMatrix) override;

    [[nodiscard]] bool AddLight(const LightComponent* lightComponent,
                                FLightComponentId lightComponentId,
                                std::unique_ptr<FLightSceneProxy> proxy) override;
    void UpdateLight(FLightComponentId lightComponentId, std::unique_ptr<FLightSceneProxy> proxy) override;
    void RemoveLight(FLightComponentId lightComponentId) override;

    [[nodiscard]] const std::vector<FPrimitiveSceneProxy*>& GetPrimitives() const { return m_Primitives; }
    [[nodiscard]] const std::vector<FLightSceneProxy*>& GetLights() const { return m_Lights; }

    [[nodiscard]] RHIPipeline* ResolvePreparedPipeline(const FPipelineKey& pipelineKey) const;
    [[nodiscard]] RHITexture* ResolvePreparedBaseColorTexture(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] RHISampler* ResolveDefaultSampler() const;
    [[nodiscard]] RHISampler* ResolveGBufferSampler() const;

    void SetViewInfo(const FViewInfo& viewInfo) { m_ViewInfo = viewInfo; }
    [[nodiscard]] const FViewInfo& GetViewInfo() const { return m_ViewInfo; }

private:
    [[nodiscard]] bool PrepareProxyResources(FPrimitiveSceneProxy& proxy);
    [[nodiscard]] bool InsertPrimitive(FPrimitiveComponentId primitiveComponentId,
                                       const PrimitiveComponent* primitiveComponent,
                                       std::unique_ptr<FPrimitiveSceneProxy> proxy);
    void RebuildPrimitiveView();
    void RebuildLightView();

    std::unique_ptr<FRenderResourceManager> m_RenderResourceManager;
    std::unordered_map<FPrimitiveComponentId, std::unique_ptr<FPrimitiveSceneInfo>, FPrimitiveComponentIdHash> m_PrimitiveStorage;
    std::unordered_map<FLightComponentId, std::unique_ptr<FLightSceneProxy>, FLightComponentIdHash> m_LightStorage;
    std::vector<FPrimitiveSceneProxy*> m_Primitives;
    std::vector<FLightSceneProxy*> m_Lights;
    FViewInfo m_ViewInfo;
};

} // namespace TE
