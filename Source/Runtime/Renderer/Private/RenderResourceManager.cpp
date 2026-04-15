// ToyEngine Renderer Module
// FRenderResourceManager 实现

#include "RenderResourceManager.h"

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

// 模型 Shader 路径前缀宏（由 CMake 传递）
#ifndef TE_PROJECT_ROOT_DIR
    #define TE_PROJECT_ROOT_DIR ""
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace TE {

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
    if (!EnsureStaticMeshPipeline())
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
            m_StaticMeshBaseColorTextureCache.erase(it->first);
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
    if (m_StaticMeshBaseColorTextureCache.find(key) != m_StaticMeshBaseColorTextureCache.end())
    {
        return true;
    }

    if (!EnsureDefaultTextureResources())
    {
        return false;
    }

    const auto& materials = staticMesh.GetMaterials();
    std::vector<std::shared_ptr<RHITexture>> textures;
    textures.reserve(materials.empty() ? 1 : materials.size());

    if (materials.empty())
    {
        textures.push_back(m_DefaultWhiteTexture);
    }
    else
    {
        for (uint32_t materialIndex = 0; materialIndex < materials.size(); ++materialIndex)
        {
            const auto& material = materials[materialIndex];
            std::shared_ptr<RHITexture> texture;
            if (material.HasBaseColorTexture())
            {
                texture = CreateTextureFromFile(material.BaseColorTexturePath,
                                                "BaseColor_" + staticMesh.GetName() + "_" + std::to_string(materialIndex));
            }

            if (!texture)
            {
                texture = m_DefaultWhiteTexture;
            }

            textures.push_back(std::move(texture));
        }
    }

    m_StaticMeshBaseColorTextureCache[key] = std::move(textures);
    return true;
}

bool FRenderResourceManager::EnsureDefaultTextureResources()
{
    if (m_DefaultWhiteTexture && m_DefaultSampler)
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

    return true;
}

std::shared_ptr<RHITexture> FRenderResourceManager::CreateTextureFromFile(const std::string& filePath,
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
    desc.srgb = true;
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

bool FRenderResourceManager::EnsureStaticMeshPipeline()
{
    if (m_StaticMeshPipeline && m_StaticMeshPipeline->IsValid())
    {
        return true;
    }

    if (!m_Device)
    {
        return false;
    }

    // 根据后端类型选择对应的着色器子目录
    const auto& traits = m_Device->GetBackendTraits();
    std::string shaderSubDir;
    switch (traits.backendType)
    {
    case ERHIBackendType::OpenGL:  shaderSubDir = "OpenGL/"; break;
    case ERHIBackendType::Vulkan:  shaderSubDir = "Vulkan/"; break;
    case ERHIBackendType::D3D12:   shaderSubDir = "D3D12/"; break;
    }
    const std::string shaderDir = std::string(TE_PROJECT_ROOT_DIR) + "Content/Shaders/" + shaderSubDir;

    RHIShaderDesc vsDesc;
    vsDesc.stage = RHIShaderStage::Vertex;
    vsDesc.filePath = shaderDir + "model.vert";
    vsDesc.debugName = "Model_VS";
    m_StaticMeshVertexShader = m_Device->CreateShader(vsDesc);

    RHIShaderDesc fsDesc;
    fsDesc.stage = RHIShaderStage::Fragment;
    fsDesc.filePath = shaderDir + "model.frag";
    fsDesc.debugName = "Model_FS";
    m_StaticMeshFragmentShader = m_Device->CreateShader(fsDesc);

    if (!m_StaticMeshVertexShader || !m_StaticMeshFragmentShader)
    {
        return false;
    }

    RHIPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = m_StaticMeshVertexShader.get();
    pipelineDesc.fragmentShader = m_StaticMeshFragmentShader.get();
    pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;

    RHIVertexBindingDesc binding;
    binding.binding = 0;
    binding.stride = sizeof(FStaticMeshVertex);
    pipelineDesc.vertexInput.bindings.push_back(binding);

    pipelineDesc.vertexInput.attributes.push_back({0, RHIFormat::Float3, offsetof(FStaticMeshVertex, Position)});
    pipelineDesc.vertexInput.attributes.push_back({1, RHIFormat::Float3, offsetof(FStaticMeshVertex, Normal)});
    pipelineDesc.vertexInput.attributes.push_back({2, RHIFormat::Float2, offsetof(FStaticMeshVertex, TexCoord)});
    pipelineDesc.vertexInput.attributes.push_back({3, RHIFormat::Float3, offsetof(FStaticMeshVertex, Color)});

    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Less;
    pipelineDesc.rasterization.cullMode = RHICullMode::Back;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.debugName = "StaticMesh_Pipeline";

    m_StaticMeshPipeline = m_Device->CreatePipeline(pipelineDesc);
    return m_StaticMeshPipeline && m_StaticMeshPipeline->IsValid();
}

RHIPipeline* FRenderResourceManager::GetPreparedPipeline(EMeshPipelineKey pipelineKey) const
{
    switch (pipelineKey)
    {
    case EMeshPipelineKey::StaticMeshLit:
        return m_StaticMeshPipeline.get();
    default:
        return nullptr;
    }
}

RHITexture* FRenderResourceManager::GetPreparedBaseColorTexture(const StaticMesh* staticMesh, uint32_t materialIndex) const
{
    if (!staticMesh)
    {
        return m_DefaultWhiteTexture.get();
    }

    const auto found = m_StaticMeshBaseColorTextureCache.find(staticMesh);
    if (found == m_StaticMeshBaseColorTextureCache.end())
    {
        return m_DefaultWhiteTexture.get();
    }

    const auto& textures = found->second;
    if (textures.empty())
    {
        return m_DefaultWhiteTexture.get();
    }

    if (materialIndex >= textures.size())
    {
        return m_DefaultWhiteTexture.get();
    }

    auto* texture = textures[materialIndex].get();
    return texture ? texture : m_DefaultWhiteTexture.get();
}

RHISampler* FRenderResourceManager::GetDefaultSampler() const
{
    return m_DefaultSampler.get();
}

} // namespace TE
