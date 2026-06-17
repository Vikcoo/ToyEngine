// ToyEngine Renderer Module
// FRenderResourceManager - 渲染侧共享资源管理

#pragma once

#include "MeshDrawCommand.h"
#include "RHIBindGroup.h"
#include "RHIPipeline.h"
#include "Material.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace TE {

class RHIDevice;
class RHIBindGroupLayout;
class RHIPipeline;
class RHIPipelineLayout;
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
    std::vector<std::unique_ptr<RHIBindGroupLayout>> BindGroupLayouts;
    std::unique_ptr<RHIPipelineLayout> PipelineLayout;
    std::unique_ptr<RHIPipeline> Pipeline;
};

struct FPreparedMaterialTextures
{
    std::shared_ptr<RHITexture> BaseColor;
    std::shared_ptr<RHITexture> Normal;
    std::shared_ptr<RHITexture> Metallic;
    std::shared_ptr<RHITexture> Roughness;
    std::shared_ptr<RHITexture> AmbientOcclusion;
    std::shared_ptr<RHITexture> Emissive;
};

struct FEnvironmentIBLResources
{
    std::shared_ptr<RHITexture> EnvironmentMap;
    std::shared_ptr<RHITexture> IrradianceMap;
    std::shared_ptr<RHITexture> PrefilterMap;
    std::shared_ptr<RHITexture> BRDFLUT;
};

class FRenderResourceManager
{
public:
    explicit FRenderResourceManager(RHIDevice* device);
    ~FRenderResourceManager();

    [[nodiscard]] bool PrepareStaticMeshProxy(FStaticMeshSceneProxy& proxy);
    [[nodiscard]] RHIPipeline* GetPreparedPipeline(const FPipelineKey& pipelineKey) const;
    [[nodiscard]] RHITexture* GetPreparedBaseColorTexture(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] const FPreparedMaterialTextures* GetPreparedMaterialTextures(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] const FMaterial* GetMaterial(const StaticMesh* staticMesh, uint32_t materialIndex) const;
    [[nodiscard]] const FEnvironmentIBLResources* GetEnvironmentIBLResources() const;
    [[nodiscard]] RHISampler* GetDefaultSampler() const;
    [[nodiscard]] RHISampler* GetEnvironmentSampler() const;
    [[nodiscard]] RHISampler* GetGBufferSampler() const;
    void PurgeExpiredStaticMeshRenderData();

private:
    [[nodiscard]] std::shared_ptr<const FStaticMeshRenderData> GetOrCreateStaticMeshRenderData(
        const std::shared_ptr<StaticMesh>& staticMesh);
    [[nodiscard]] bool EnsureStaticMeshMaterialTextures(const StaticMesh& staticMesh);
    [[nodiscard]] bool EnsureDefaultTextureResources();
    [[nodiscard]] bool EnsureEnvironmentResources();
    [[nodiscard]] std::shared_ptr<RHITexture> GetOrCreateTextureFromSlot(const FMaterialTextureSlot& textureSlot,
                                                                         const std::string& debugName);
    [[nodiscard]] std::shared_ptr<RHITexture> CreateTextureFromFile(const std::string& filePath,
                                                                    ETextureColorSpace colorSpace,
                                                                    const std::string& debugName) const;
    [[nodiscard]] bool EnsurePipeline(const FPipelineKey& pipelineKey);
    [[nodiscard]] bool BuildStaticMeshBasePassPipeline(FPreparedPipeline& outPipeline);

    RHIDevice* m_Device = nullptr;
    std::unordered_map<const StaticMesh*, std::weak_ptr<const FStaticMeshRenderData>> m_StaticMeshRenderDataCache;
    std::unordered_map<const StaticMesh*, std::vector<FPreparedMaterialTextures>> m_StaticMeshMaterialTextureCache;
    std::unordered_map<std::string, std::weak_ptr<RHITexture>> m_TextureCache;
    std::unordered_map<FPipelineKey, FPreparedPipeline, FPipelineKeyHash> m_PipelineCache;
    std::shared_ptr<RHITexture> m_DefaultWhiteTexture;
    std::shared_ptr<RHITexture> m_DefaultBlackTexture;
    std::shared_ptr<RHITexture> m_DefaultNormalTexture;
    FEnvironmentIBLResources m_EnvironmentIBLResources;
    std::shared_ptr<RHISampler> m_DefaultSampler;
    std::shared_ptr<RHISampler> m_EnvironmentSampler;
    std::shared_ptr<RHISampler> m_GBufferSampler;
};

} // namespace TE
