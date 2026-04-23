// ToyEngine Renderer Module
// FRenderResourceManager - 渲染侧共享资源管理

#pragma once

#include "MeshDrawCommand.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace TE {

class RHIDevice;
class RHIPipeline;
class RHIShader;
class RHITexture;
class RHISampler;
class StaticMesh;
class FStaticMeshRenderData;
class FStaticMeshSceneProxy;

/// 管理渲染侧共享资源（静态网格 RenderData 与共享 Pipeline）。
/// 该层负责把 SceneProxy 的资产描述解析为可用 GPU 资源。
struct FPreparedPipeline
{
    std::unique_ptr<RHIShader> VertexShader;
    std::unique_ptr<RHIShader> FragmentShader;
    std::unique_ptr<RHIPipeline> Pipeline;
};

class FRenderResourceManager
{
public:
    explicit FRenderResourceManager(RHIDevice* device);
    ~FRenderResourceManager();

    [[nodiscard]] bool PrepareStaticMeshProxy(FStaticMeshSceneProxy& proxy);
    [[nodiscard]] RHIPipeline* GetPreparedPipeline(const FPipelineKey& pipelineKey) const;
    [[nodiscard]] RHITexture* GetPreparedBaseColorTexture(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] RHISampler* GetDefaultSampler() const;
    [[nodiscard]] RHISampler* GetGBufferSampler() const;
    void PurgeExpiredStaticMeshRenderData();

private:
    [[nodiscard]] std::shared_ptr<const FStaticMeshRenderData> GetOrCreateStaticMeshRenderData(
        const std::shared_ptr<StaticMesh>& staticMesh);
    [[nodiscard]] bool EnsureStaticMeshMaterialTextures(const StaticMesh& staticMesh);
    [[nodiscard]] bool EnsureDefaultTextureResources();
    [[nodiscard]] std::shared_ptr<RHITexture> CreateTextureFromFile(const std::string& filePath, const std::string& debugName) const;
    [[nodiscard]] bool EnsurePipeline(const FPipelineKey& pipelineKey);
    [[nodiscard]] bool BuildStaticMeshBasePassPipeline(FPreparedPipeline& outPipeline);

    RHIDevice* m_Device = nullptr;
    std::unordered_map<const StaticMesh*, std::weak_ptr<const FStaticMeshRenderData>> m_StaticMeshRenderDataCache;
    std::unordered_map<const StaticMesh*, std::vector<std::shared_ptr<RHITexture>>> m_StaticMeshBaseColorTextureCache;
    std::unordered_map<FPipelineKey, FPreparedPipeline, FPipelineKeyHash> m_PipelineCache;
    std::shared_ptr<RHITexture> m_DefaultWhiteTexture;
    std::shared_ptr<RHISampler> m_DefaultSampler;
    std::shared_ptr<RHISampler> m_GBufferSampler;
};

} // namespace TE
