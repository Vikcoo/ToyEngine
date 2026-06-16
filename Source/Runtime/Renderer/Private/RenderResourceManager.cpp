// ToyEngine Renderer Module
// FRenderResourceManager 实现

#include "RenderResourceManager.h"

#include "RendererBindingSlots.h"
#include "RendererShaderNames.h"
#include "Material.h"
#include "StaticMeshRenderData.h"
#include "StaticMeshSceneProxy.h"
#include "RHIDevice.h"
#include "RHIPipeline.h"
#include "RHISampler.h"
#include "RHIShader.h"
#include "RHITexture.h"
#include "RHITypes.h"
#include "StaticMesh.h"
#include "Log/Log.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace TE {

namespace {

std::unique_ptr<RHIBindGroupLayout> CreateSingleUniformLayout(RHIDevice* device,
                                                              uint32_t binding,
                                                              RHIShaderStage visibility,
                                                              const char* debugName)
{
    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({
        binding,
        RHIBindingType::UniformBuffer,
        visibility
    });
    return device ? device->CreateBindGroupLayout(desc) : nullptr;
}

std::unique_ptr<RHIBindGroupLayout> CreateSingleTextureLayout(RHIDevice* device,
                                                              uint32_t binding,
                                                              const char* debugName)
{
    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({
        binding,
        RHIBindingType::Texture2D,
        RHIShaderStage::Fragment
    });
    return device ? device->CreateBindGroupLayout(desc) : nullptr;
}

std::unique_ptr<RHIBindGroupLayout> CreateMaterialTexturesLayout(RHIDevice* device, const char* debugName)
{
    if (!device)
    {
        return nullptr;
    }

    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({RendererBindings::BaseColorTexture, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::NormalTexture, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::MetallicTexture, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::RoughnessTexture, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::AOTexture, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::EmissiveTexture, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    return device->CreateBindGroupLayout(desc);
}

bool BuildPipelineLayout(RHIDevice* device,
                         FPreparedPipeline& pipeline,
                         std::vector<std::pair<uint32_t, std::unique_ptr<RHIBindGroupLayout>>> layouts,
                         const char* debugName)
{
    if (!device)
    {
        return false;
    }

    RHIPipelineLayoutDesc desc;
    desc.debugName = debugName;
    desc.bindGroupLayouts.reserve(layouts.size());
    for (const auto& [groupIndex, layout] : layouts)
    {
        if (!layout || !layout->IsValid())
        {
            return false;
        }
        desc.bindGroupLayouts.push_back({groupIndex, layout.get()});
    }

    auto pipelineLayout = device->CreatePipelineLayout(desc);
    if (!pipelineLayout || !pipelineLayout->IsValid())
    {
        return false;
    }

    pipeline.BindGroupLayouts.clear();
    pipeline.BindGroupLayouts.reserve(layouts.size());
    for (auto& [groupIndex, layout] : layouts)
    {
        (void)groupIndex;
        pipeline.BindGroupLayouts.push_back(std::move(layout));
    }
    pipeline.PipelineLayout = std::move(pipelineLayout);
    return true;
}

} // namespace

FRenderResourceManager::FRenderResourceManager(RHIDevice* device)
    : m_Device(device)
{
}

FRenderResourceManager::~FRenderResourceManager() = default;

bool FRenderResourceManager::PrepareStaticMeshProxy(FStaticMeshSceneProxy& proxy)
{
    const auto& staticMesh = proxy.GetStaticMeshAsset();
    if (!staticMesh || !staticMesh->IsValid())
    {
        return false;
    }

    auto renderData = GetOrCreateStaticMeshRenderData(staticMesh);
    if (!renderData)
    {
        return false;
    }

    // 在 Primitive 注册阶段预热 pipeline，保证后续渲染阶段可直接解析。
    if (!EnsurePipeline(FPipelineKey::StaticMeshBasePass()))
    {
        return false;
    }

    if (!EnsureStaticMeshMaterialTextures(*staticMesh))
    {
        return false;
    }

    return proxy.SetRenderResources(std::move(renderData));
}

void FRenderResourceManager::PurgeExpiredStaticMeshRenderData()
{
    for (auto it = m_StaticMeshRenderDataCache.begin(); it != m_StaticMeshRenderDataCache.end();)
    {
        if (it->second.expired())
        {
            m_StaticMeshMaterialTextureCache.erase(it->first);
            it = m_StaticMeshRenderDataCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::shared_ptr<const FStaticMeshRenderData> FRenderResourceManager::GetOrCreateStaticMeshRenderData(
    const std::shared_ptr<StaticMesh>& staticMesh)
{
    if (!m_Device || !staticMesh)
    {
        return nullptr;
    }

    const StaticMesh* key = staticMesh.get();
    const auto found = m_StaticMeshRenderDataCache.find(key);
    if (found != m_StaticMeshRenderDataCache.end())
    {
        auto cached = found->second.lock();
        if (cached && cached->IsValid())
        {
            return cached;
        }
    }

    auto newRenderData = FStaticMeshRenderData::Create(*staticMesh, *m_Device);
    if (!newRenderData || !newRenderData->IsValid())
    {
        return nullptr;
    }

    m_StaticMeshRenderDataCache[key] = newRenderData;
    return newRenderData;
}

bool FRenderResourceManager::EnsureStaticMeshMaterialTextures(const StaticMesh& staticMesh)
{
    const StaticMesh* key = &staticMesh;
    if (m_StaticMeshMaterialTextureCache.find(key) != m_StaticMeshMaterialTextureCache.end())
    {
        return true;
    }

    if (!EnsureDefaultTextureResources())
    {
        return false;
    }

    const auto& materials = staticMesh.GetMaterials();
    std::vector<FPreparedMaterialTextures> textures;
    textures.reserve(materials.empty() ? 1 : materials.size());

    if (materials.empty())
    {
        FPreparedMaterialTextures defaultTextures;
        defaultTextures.BaseColor = m_DefaultWhiteTexture;
        defaultTextures.Normal = m_DefaultNormalTexture;
        defaultTextures.Metallic = m_DefaultBlackTexture;
        defaultTextures.Roughness = m_DefaultWhiteTexture;
        defaultTextures.AmbientOcclusion = m_DefaultWhiteTexture;
        defaultTextures.Emissive = m_DefaultBlackTexture;
        textures.push_back(std::move(defaultTextures));
    }
    else
    {
        for (uint32_t materialIndex = 0; materialIndex < materials.size(); ++materialIndex)
        {
            const auto& material = materials[materialIndex];
            const std::string materialDebugName = staticMesh.GetName() + "_" + std::to_string(materialIndex);

            FPreparedMaterialTextures preparedTextures;
            preparedTextures.BaseColor = GetOrCreateTextureFromSlot(material.BaseColorTexture,
                                                                    "BaseColor_" + materialDebugName);
            preparedTextures.Normal = GetOrCreateTextureFromSlot(material.NormalTexture,
                                                                 "Normal_" + materialDebugName);
            preparedTextures.Metallic = GetOrCreateTextureFromSlot(material.MetallicTexture,
                                                                   "Metallic_" + materialDebugName);
            preparedTextures.Roughness = GetOrCreateTextureFromSlot(material.RoughnessTexture,
                                                                    "Roughness_" + materialDebugName);
            preparedTextures.AmbientOcclusion = GetOrCreateTextureFromSlot(material.AmbientOcclusionTexture,
                                                                          "AO_" + materialDebugName);
            preparedTextures.Emissive = GetOrCreateTextureFromSlot(material.EmissiveTexture,
                                                                   "Emissive_" + materialDebugName);

            if (!preparedTextures.BaseColor) preparedTextures.BaseColor = m_DefaultWhiteTexture;
            if (!preparedTextures.Normal) preparedTextures.Normal = m_DefaultNormalTexture;
            if (!preparedTextures.Metallic) preparedTextures.Metallic = m_DefaultBlackTexture;
            if (!preparedTextures.Roughness) preparedTextures.Roughness = m_DefaultWhiteTexture;
            if (!preparedTextures.AmbientOcclusion) preparedTextures.AmbientOcclusion = m_DefaultWhiteTexture;
            if (!preparedTextures.Emissive) preparedTextures.Emissive = m_DefaultBlackTexture;

            textures.push_back(std::move(preparedTextures));
        }
    }

    m_StaticMeshMaterialTextureCache[key] = std::move(textures);
    return true;
}

bool FRenderResourceManager::EnsureDefaultTextureResources()
{
    if (m_DefaultWhiteTexture && m_DefaultBlackTexture && m_DefaultNormalTexture && m_DefaultSampler && m_GBufferSampler)
    {
        return true;
    }

    if (!m_Device)
    {
        return false;
    }

    if (!m_DefaultWhiteTexture)
    {
        const uint8_t whitePixel[4] = {255, 255, 255, 255};
        RHITextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.format = RHIFormat::RGBA8_UNorm;
        desc.initialData = whitePixel;
        desc.generateMips = false;
        desc.srgb = true;
        desc.debugName = "Default_White_Texture";

        auto texture = m_Device->CreateTexture(desc);
        if (!texture || !texture->IsValid())
        {
            TE_LOG_ERROR("[Renderer] Failed to create default white texture");
            return false;
        }
        m_DefaultWhiteTexture = std::shared_ptr<RHITexture>(texture.release());
    }

    if (!m_DefaultBlackTexture)
    {
        const uint8_t blackPixel[4] = {0, 0, 0, 255};
        RHITextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.format = RHIFormat::RGBA8_UNorm;
        desc.initialData = blackPixel;
        desc.generateMips = false;
        desc.srgb = false;
        desc.debugName = "Default_Black_Texture";

        auto texture = m_Device->CreateTexture(desc);
        if (!texture || !texture->IsValid())
        {
            TE_LOG_ERROR("[Renderer] Failed to create default black texture");
            return false;
        }
        m_DefaultBlackTexture = std::shared_ptr<RHITexture>(texture.release());
    }

    if (!m_DefaultNormalTexture)
    {
        const uint8_t normalPixel[4] = {128, 128, 255, 255};
        RHITextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.format = RHIFormat::RGBA8_UNorm;
        desc.initialData = normalPixel;
        desc.generateMips = false;
        desc.srgb = false;
        desc.debugName = "Default_Normal_Texture";

        auto texture = m_Device->CreateTexture(desc);
        if (!texture || !texture->IsValid())
        {
            TE_LOG_ERROR("[Renderer] Failed to create default normal texture");
            return false;
        }
        m_DefaultNormalTexture = std::shared_ptr<RHITexture>(texture.release());
    }

    if (!m_DefaultSampler)
    {
        RHISamplerDesc desc;
        desc.minFilter = RHITextureFilter::Linear;
        desc.magFilter = RHITextureFilter::Linear;
        desc.addressU = RHITextureAddressMode::Repeat;
        desc.addressV = RHITextureAddressMode::Repeat;
        desc.debugName = "Default_Linear_Sampler";

        auto sampler = m_Device->CreateSampler(desc);
        if (!sampler || !sampler->IsValid())
        {
            TE_LOG_ERROR("[Renderer] Failed to create default sampler");
            return false;
        }
        m_DefaultSampler = std::shared_ptr<RHISampler>(sampler.release());
    }

    if (!m_GBufferSampler)
    {
        RHISamplerDesc desc;
        desc.minFilter = RHITextureFilter::Nearest;
        desc.magFilter = RHITextureFilter::Nearest;
        desc.addressU = RHITextureAddressMode::ClampToEdge;
        desc.addressV = RHITextureAddressMode::ClampToEdge;
        desc.debugName = "GBuffer_Nearest_Sampler";

        auto sampler = m_Device->CreateSampler(desc);
        if (!sampler || !sampler->IsValid())
        {
            TE_LOG_ERROR("[Renderer] Failed to create gbuffer sampler");
            return false;
        }
        m_GBufferSampler = std::shared_ptr<RHISampler>(sampler.release());
    }

    return true;
}

std::shared_ptr<RHITexture> FRenderResourceManager::GetOrCreateTextureFromSlot(const FMaterialTextureSlot& textureSlot,
                                                                                const std::string& debugName)
{
    if (!textureSlot.HasTexture())
    {
        return nullptr;
    }

    const std::filesystem::path normalizedPath = std::filesystem::path(textureSlot.TexturePath).lexically_normal();
    const std::string cacheKey = normalizedPath.string() + "|" +
        (textureSlot.ColorSpace == ETextureColorSpace::SRGB ? "srgb" : "linear") + "|" +
        std::to_string(static_cast<uint32_t>(textureSlot.Semantic));

    const auto found = m_TextureCache.find(cacheKey);
    if (found != m_TextureCache.end())
    {
        if (auto cached = found->second.lock())
        {
            return cached;
        }
    }

    auto texture = CreateTextureFromFile(normalizedPath.string(), textureSlot.ColorSpace, debugName);
    if (texture)
    {
        m_TextureCache[cacheKey] = texture;
    }
    return texture;
}

std::shared_ptr<RHITexture> FRenderResourceManager::CreateTextureFromFile(const std::string& filePath,
                                                                           ETextureColorSpace colorSpace,
                                                                           const std::string& debugName) const
{
    if (!m_Device || filePath.empty())
    {
        return nullptr;
    }

    if (!std::filesystem::exists(filePath))
    {
        TE_LOG_WARN("[Renderer] Texture file not found: {}", filePath);
        return nullptr;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels || width <= 0 || height <= 0)
    {
        TE_LOG_WARN("[Renderer] Failed to load texture '{}': {}", filePath, stbi_failure_reason());
        if (pixels)
        {
            stbi_image_free(pixels);
        }
        return nullptr;
    }

    RHITextureDesc desc;
    desc.width = static_cast<uint32_t>(width);
    desc.height = static_cast<uint32_t>(height);
    desc.format = RHIFormat::RGBA8_UNorm;
    desc.initialData = pixels;
    desc.generateMips = true;
    desc.srgb = colorSpace == ETextureColorSpace::SRGB;
    desc.debugName = debugName;

    auto texture = m_Device->CreateTexture(desc);
    stbi_image_free(pixels);
    if (!texture || !texture->IsValid())
    {
        TE_LOG_WARN("[Renderer] Failed to create GPU texture for '{}'", filePath);
        return nullptr;
    }

    return std::shared_ptr<RHITexture>(texture.release());
}

bool FRenderResourceManager::EnsurePipeline(const FPipelineKey& pipelineKey)
{
    const auto found = m_PipelineCache.find(pipelineKey);
    if (found != m_PipelineCache.end())
    {
        const auto* pipeline = found->second.Pipeline.get();
        return pipeline && pipeline->IsValid();
    }

    FPreparedPipeline preparedPipeline;
    if (pipelineKey == FPipelineKey::StaticMeshBasePass())
    {
        if (!BuildStaticMeshBasePassPipeline(preparedPipeline))
        {
            return false;
        }
    }
    else
    {
        TE_LOG_WARN("[Renderer] Unsupported pipeline key: pass={}, domain={}, vertexFactory={}",
                    static_cast<uint32_t>(pipelineKey.Pass),
                    static_cast<uint32_t>(pipelineKey.MaterialDomain),
                    static_cast<uint32_t>(pipelineKey.VertexFactory));
        return false;
    }

    m_PipelineCache.emplace(pipelineKey, std::move(preparedPipeline));
    return true;
}

bool FRenderResourceManager::BuildStaticMeshBasePassPipeline(FPreparedPipeline& outPipeline)
{
    if (!m_Device)
    {
        return false;
    }

    RHIShaderDesc vsDesc;
    vsDesc.stage = RHIShaderStage::Vertex;
    vsDesc.logicalName = RendererShaderNames::StaticMeshBasePassVS;
    vsDesc.debugName = "Model_VS";
    outPipeline.VertexShader = m_Device->CreateShader(vsDesc);

    RHIShaderDesc fsDesc;
    fsDesc.stage = RHIShaderStage::Fragment;
    fsDesc.logicalName = RendererShaderNames::StaticMeshBasePassPS;
    fsDesc.debugName = "Model_FS";
    outPipeline.FragmentShader = m_Device->CreateShader(fsDesc);

    if (!outPipeline.VertexShader || !outPipeline.FragmentShader)
    {
        return false;
    }

    std::vector<std::pair<uint32_t, std::unique_ptr<RHIBindGroupLayout>>> layouts;
    layouts.push_back({
        RendererBindGroups::LightBlock,
        CreateSingleUniformLayout(m_Device,
                                  RendererBindings::LightBlock,
                                  RHIShaderStage::Fragment,
                                  "StaticMeshBasePass_LightBlock_Layout")
    });
    layouts.push_back({
        RendererBindGroups::PassBlock,
        CreateSingleUniformLayout(m_Device,
                                  RendererBindings::PassBlock,
                                  RHIShaderStage::Vertex,
                                  "StaticMeshBasePass_ObjectBlock_Layout")
    });
    layouts.push_back({
        RendererBindGroups::MaterialTextures,
        CreateMaterialTexturesLayout(m_Device, "StaticMeshBasePass_MaterialTextures_Layout")
    });
    layouts.push_back({
        RendererBindGroups::MaterialBlock,
        CreateSingleUniformLayout(m_Device,
                                  RendererBindings::MaterialBlock,
                                  RHIShaderStage::Fragment,
                                  "StaticMeshBasePass_MaterialBlock_Layout")
    });
    if (!BuildPipelineLayout(m_Device, outPipeline, std::move(layouts), "StaticMeshBasePass_PipelineLayout"))
    {
        return false;
    }

    RHIPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = outPipeline.VertexShader.get();
    pipelineDesc.fragmentShader = outPipeline.FragmentShader.get();
    pipelineDesc.layout = outPipeline.PipelineLayout.get();
    pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;

    RHIVertexBindingDesc binding;
    binding.binding = 0;
    binding.stride = sizeof(FStaticMeshVertex);
    pipelineDesc.vertexInput.bindings.push_back(binding);

    pipelineDesc.vertexInput.attributes.push_back({0, RHIFormat::Float3, offsetof(FStaticMeshVertex, Position)});
    pipelineDesc.vertexInput.attributes.push_back({1, RHIFormat::Float3, offsetof(FStaticMeshVertex, Normal)});
    pipelineDesc.vertexInput.attributes.push_back({2, RHIFormat::Float2, offsetof(FStaticMeshVertex, TexCoord)});
    pipelineDesc.vertexInput.attributes.push_back({3, RHIFormat::Float3, offsetof(FStaticMeshVertex, Color)});
    pipelineDesc.vertexInput.attributes.push_back({4, RHIFormat::Float3, offsetof(FStaticMeshVertex, Tangent)});

    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Less;
    pipelineDesc.rasterization.cullMode = RHICullMode::Back;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.debugName = "StaticMesh_Pipeline";

    outPipeline.Pipeline = m_Device->CreatePipeline(pipelineDesc);
    return outPipeline.Pipeline && outPipeline.Pipeline->IsValid();
}

RHIPipeline* FRenderResourceManager::GetPreparedPipeline(const FPipelineKey& pipelineKey) const
{
    const auto found = m_PipelineCache.find(pipelineKey);
    if (found == m_PipelineCache.end())
    {
        return nullptr;
    }

    return found->second.Pipeline.get();
}

RHITexture* FRenderResourceManager::GetPreparedBaseColorTexture(const StaticMesh* staticMesh, uint32_t materialIndex) const
{
    const auto* textures = GetPreparedMaterialTextures(staticMesh, materialIndex);
    return textures && textures->BaseColor ? textures->BaseColor.get() : m_DefaultWhiteTexture.get();
}

const FPreparedMaterialTextures* FRenderResourceManager::GetPreparedMaterialTextures(const StaticMesh* staticMesh,
                                                                                     uint32_t materialIndex) const
{
    if (!staticMesh)
    {
        return nullptr;
    }

    const auto found = m_StaticMeshMaterialTextureCache.find(staticMesh);
    if (found == m_StaticMeshMaterialTextureCache.end())
    {
        return nullptr;
    }
    const auto& textures = found->second;
    if (textures.empty())
    {
        return nullptr;
    }

    if (materialIndex >= textures.size())
    {
        return &textures.front();
    }

    return &textures[materialIndex];
}

const FMaterial* FRenderResourceManager::GetMaterial(const StaticMesh* staticMesh, uint32_t materialIndex) const
{
    if (!staticMesh)
    {
        return nullptr;
    }

    const auto& materials = staticMesh->GetMaterials();
    if (materials.empty())
    {
        return nullptr;
    }

    if (materialIndex >= materials.size())
    {
        return &materials.front();
    }

    return &materials[materialIndex];
}

RHISampler* FRenderResourceManager::GetDefaultSampler() const
{
    return m_DefaultSampler.get();
}

RHISampler* FRenderResourceManager::GetGBufferSampler() const
{
    return m_GBufferSampler.get();
}

} // namespace TE
