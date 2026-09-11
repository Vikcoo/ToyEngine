// ToyEngine Renderer Module
// FScene - 渲染侧场景容器

#pragma once

#include "LightComponentId.h"
#include "LightSceneProxy.h"
#include "PrimitiveComponentId.h"
#include "PrimitiveSceneInfo.h"
#include "PrimitiveSceneProxy.h"
#include "MeshDrawCommand.h"
#include "RenderSceneCommand.h"
#include "ViewInfo.h"

#include <memory>
#include <unordered_map>
#include <vector>

namespace TE {

class FRenderResourceManager;
class RHIDevice;
class RHIPipeline;
class RHISampler;
class RHITexture;
class StaticMesh;
struct FMaterial;
struct FPreparedMaterialTextures;
struct FEnvironmentIBLResources;

class FScene
{
public:
    explicit FScene(RHIDevice* device);
    ~FScene();

    /**
     * 按记录顺序消费一批渲染场景命令。
     * @note 只能由渲染阶段调用。
     */
    void ApplyCommands(std::vector<FRenderSceneCommand> commands);

    [[nodiscard]] const std::vector<FPrimitiveSceneProxy*>& GetPrimitives() const { return m_Primitives; }
    [[nodiscard]] const std::vector<FLightSceneProxy*>& GetLights() const { return m_Lights; }

    [[nodiscard]] RHIPipeline* ResolvePreparedPipeline(const FPipelineKey& pipelineKey) const;
    [[nodiscard]] RHITexture* ResolvePreparedBaseColorTexture(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] const FPreparedMaterialTextures* ResolvePreparedMaterialTextures(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] const FMaterial* ResolveMaterial(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] const FEnvironmentIBLResources* ResolveEnvironmentIBLResources() const;
    [[nodiscard]] RHISampler* ResolveDefaultSampler() const;
    [[nodiscard]] RHISampler* ResolveEnvironmentSampler() const;
    [[nodiscard]] RHISampler* ResolveGBufferSampler() const;

    void SetViewInfo(const FViewInfo& viewInfo) { m_ViewInfo = viewInfo; }
    [[nodiscard]] const FViewInfo& GetViewInfo() const { return m_ViewInfo; }

private:
    [[nodiscard]] bool AddPrimitive(FPrimitiveComponentId primitiveComponentId,
                                    std::unique_ptr<FPrimitiveSceneProxy> proxy);
    void RemovePrimitive(FPrimitiveComponentId primitiveComponentId);
    void UpdatePrimitiveTransform(FPrimitiveComponentId primitiveComponentId, const Matrix4& worldMatrix);
    [[nodiscard]] bool AddLight(FLightComponentId lightComponentId,
                                std::unique_ptr<FLightSceneProxy> proxy);
    void UpdateLight(FLightComponentId lightComponentId, std::unique_ptr<FLightSceneProxy> proxy);
    void RemoveLight(FLightComponentId lightComponentId);
    [[nodiscard]] bool PrepareProxyResources(FPrimitiveSceneProxy& proxy);
    [[nodiscard]] bool InsertPrimitive(FPrimitiveComponentId primitiveComponentId,
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
