// ToyEngine Renderer Module
// FRenderResourceManager 实现

#include "RenderResourceManager.h"

#include "RendererBindingSlots.h"
#include "RendererDepthConvention.h"
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
#include "Math/ScalarMath.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace TE {

namespace {

constexpr uint32_t EnvironmentCubeSize = 128;
constexpr uint32_t IrradianceCubeSize = 32;
constexpr uint32_t BRDFLUTSize = 128;
constexpr uint32_t IrradiancePhiSamples = 48;
constexpr uint32_t IrradianceThetaSamples = 16;
constexpr uint32_t BRDFSampleCount = 128;

struct FHDRImage
{
    int Width = 0;
    int Height = 0;
    std::vector<float> Pixels;
};

[[nodiscard]] Vector3 ClampColor(const Vector3& color)
{
    return Vector3(std::max(color.X, 0.0f), std::max(color.Y, 0.0f), std::max(color.Z, 0.0f));
}

[[nodiscard]] Vector3 SampleHDRBilinear(const FHDRImage& image, float u, float v)
{
    if (image.Width <= 0 || image.Height <= 0 || image.Pixels.empty())
    {
        return Vector3::Zero;
    }

    u = u - std::floor(u);
    v = Math::Clamp(v, 0.0f, 1.0f);

    const float x = u * static_cast<float>(image.Width - 1);
    const float y = v * static_cast<float>(image.Height - 1);
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const int x1 = (x0 + 1) % image.Width;
    const int y1 = std::min(y0 + 1, image.Height - 1);
    const float tx = x - static_cast<float>(x0);
    const float ty = y - static_cast<float>(y0);

    auto fetch = [&image](int px, int py)
    {
        const size_t index = (static_cast<size_t>(py) * image.Width + px) * 4;
        return Vector3(image.Pixels[index + 0], image.Pixels[index + 1], image.Pixels[index + 2]);
    };

    const Vector3 c00 = fetch(x0, y0);
    const Vector3 c10 = fetch(x1, y0);
    const Vector3 c01 = fetch(x0, y1);
    const Vector3 c11 = fetch(x1, y1);
    const Vector3 cx0 = Vector3::Lerp(c00, c10, tx);
    const Vector3 cx1 = Vector3::Lerp(c01, c11, tx);
    return ClampColor(Vector3::Lerp(cx0, cx1, ty));
}

[[nodiscard]] Vector3 SampleEquirectangularHDR(const FHDRImage& image, const Vector3& direction)
{
    const Vector3 dir = direction.Normalize();
    const float u = std::atan2(dir.Z, dir.X) / Math::TWO_PI + 0.5f;
    const float v = 0.5f - std::asin(Math::Clamp(dir.Y, -1.0f, 1.0f)) / Math::PI;
    return SampleHDRBilinear(image, u, v);
}

[[nodiscard]] Vector3 GetCubeFaceDirection(uint32_t face, float u, float v)
{
    const float x = 2.0f * u - 1.0f;
    const float y = 2.0f * v - 1.0f;
    switch (face)
    {
    case 0: return Vector3(1.0f, -y, -x).Normalize();   // +X
    case 1: return Vector3(-1.0f, -y, x).Normalize();   // -X
    case 2: return Vector3(x, 1.0f, y).Normalize();     // +Y
    case 3: return Vector3(x, -1.0f, -y).Normalize();   // -Y
    case 4: return Vector3(x, -y, 1.0f).Normalize();    // +Z
    default: return Vector3(-x, -y, -1.0f).Normalize(); // -Z
    }
}

void StoreRGBA(std::vector<float>& pixels, size_t pixelIndex, const Vector3& color, float alpha = 1.0f)
{
    const size_t index = pixelIndex * 4;
    pixels[index + 0] = color.X;
    pixels[index + 1] = color.Y;
    pixels[index + 2] = color.Z;
    pixels[index + 3] = alpha;
}

[[nodiscard]] std::vector<float> GenerateEnvironmentCubePixels(const FHDRImage& image, uint32_t size)
{
    std::vector<float> pixels(static_cast<size_t>(size) * size * 6 * 4, 0.0f);
    for (uint32_t face = 0; face < 6; ++face)
    {
        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);
                const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
                const Vector3 dir = GetCubeFaceDirection(face, u, v);
                const size_t pixelIndex = (static_cast<size_t>(face) * size * size) + (static_cast<size_t>(y) * size + x);
                StoreRGBA(pixels, pixelIndex, SampleEquirectangularHDR(image, dir));
            }
        }
    }
    return pixels;
}

void BuildTangentBasis(const Vector3& n, Vector3& outTangent, Vector3& outBitangent)
{
    const Vector3 up = std::abs(n.Y) < 0.999f ? Vector3::Up : Vector3::Right;
    outTangent = Vector3::Cross(up, n).Normalize();
    outBitangent = Vector3::Cross(n, outTangent).Normalize();
}

[[nodiscard]] std::vector<float> GenerateIrradianceCubePixels(const FHDRImage& image, uint32_t size)
{
    std::vector<float> pixels(static_cast<size_t>(size) * size * 6 * 4, 0.0f);
    for (uint32_t face = 0; face < 6; ++face)
    {
        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                const float u = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);
                const float v = (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
                const Vector3 n = GetCubeFaceDirection(face, u, v);
                Vector3 tangent;
                Vector3 bitangent;
                BuildTangentBasis(n, tangent, bitangent);

                Vector3 irradiance = Vector3::Zero;
                float weight = 0.0f;
                for (uint32_t phiIndex = 0; phiIndex < IrradiancePhiSamples; ++phiIndex)
                {
                    const float phi = (static_cast<float>(phiIndex) + 0.5f) / static_cast<float>(IrradiancePhiSamples) * Math::TWO_PI;
                    for (uint32_t thetaIndex = 0; thetaIndex < IrradianceThetaSamples; ++thetaIndex)
                    {
                        const float theta = (static_cast<float>(thetaIndex) + 0.5f) / static_cast<float>(IrradianceThetaSamples) * Math::HALF_PI;
                        const float sinTheta = std::sin(theta);
                        const float cosTheta = std::cos(theta);
                        const Vector3 sampleDir = (tangent * (std::cos(phi) * sinTheta) +
                                                   bitangent * (std::sin(phi) * sinTheta) +
                                                   n * cosTheta).Normalize();
                        const float sampleWeight = cosTheta * sinTheta;
                        irradiance += SampleEquirectangularHDR(image, sampleDir) * sampleWeight;
                        weight += sampleWeight;
                    }
                }
                if (weight > 0.0f)
                {
                    irradiance = irradiance * (Math::PI / weight);
                }

                const size_t pixelIndex = (static_cast<size_t>(face) * size * size) + (static_cast<size_t>(y) * size + x);
                StoreRGBA(pixels, pixelIndex, irradiance);
            }
        }
    }
    return pixels;
}

float RadicalInverseVdc(uint32_t bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
}

[[nodiscard]] Vector2 Hammersley(uint32_t i, uint32_t n)
{
    return Vector2(static_cast<float>(i) / static_cast<float>(n), RadicalInverseVdc(i));
}

[[nodiscard]] Vector3 ImportanceSampleGGX(const Vector2& xi, const Vector3& n, float roughness)
{
    const float a = roughness * roughness;
    const float phi = Math::TWO_PI * xi.X;
    const float cosTheta = std::sqrt((1.0f - xi.Y) / (1.0f + (a * a - 1.0f) * xi.Y));
    const float sinTheta = std::sqrt(std::max(1.0f - cosTheta * cosTheta, 0.0f));
    Vector3 tangent;
    Vector3 bitangent;
    BuildTangentBasis(n, tangent, bitangent);
    return (tangent * (std::cos(phi) * sinTheta) +
            bitangent * (std::sin(phi) * sinTheta) +
            n * cosTheta).Normalize();
}

float GeometrySchlickGGX(float nDotV, float roughness)
{
    const float a = roughness;
    const float k = (a * a) / 2.0f;
    return nDotV / std::max(nDotV * (1.0f - k) + k, 0.0001f);
}

float GeometrySmithIBL(float nDotV, float nDotL, float roughness)
{
    return GeometrySchlickGGX(nDotV, roughness) * GeometrySchlickGGX(nDotL, roughness);
}

[[nodiscard]] Vector2 IntegrateBRDF(float nDotV, float roughness)
{
    const Vector3 v(std::sqrt(std::max(1.0f - nDotV * nDotV, 0.0f)), 0.0f, nDotV);
    const Vector3 n(0.0f, 0.0f, 1.0f);
    float a = 0.0f;
    float b = 0.0f;
    for (uint32_t i = 0; i < BRDFSampleCount; ++i)
    {
        const Vector2 xi = Hammersley(i, BRDFSampleCount);
        const Vector3 h = ImportanceSampleGGX(xi, n, roughness);
        const Vector3 l = (h * (2.0f * Vector3::Dot(v, h)) - v).Normalize();
        const float nDotL = std::max(l.Z, 0.0f);
        const float nDotH = std::max(h.Z, 0.0f);
        const float vDotH = std::max(Vector3::Dot(v, h), 0.0f);
        if (nDotL > 0.0f)
        {
            const float g = GeometrySmithIBL(nDotV, nDotL, roughness);
            const float gVis = (g * vDotH) / std::max(nDotH * nDotV, 0.0001f);
            const float fc = std::pow(1.0f - vDotH, 5.0f);
            a += (1.0f - fc) * gVis;
            b += fc * gVis;
        }
    }
    return Vector2(a / static_cast<float>(BRDFSampleCount), b / static_cast<float>(BRDFSampleCount));
}

[[nodiscard]] std::vector<float> GenerateBRDFLUTPixels(uint32_t size)
{
    std::vector<float> pixels(static_cast<size_t>(size) * size * 4, 0.0f);
    for (uint32_t y = 0; y < size; ++y)
    {
        for (uint32_t x = 0; x < size; ++x)
        {
            const float nDotV = (static_cast<float>(x) + 0.5f) / static_cast<float>(size);
            const float roughness = 1.0f - (static_cast<float>(y) + 0.5f) / static_cast<float>(size);
            const Vector2 brdf = IntegrateBRDF(nDotV, roughness);
            const size_t index = (static_cast<size_t>(y) * size + x) * 4;
            pixels[index + 0] = brdf.X;
            pixels[index + 1] = brdf.Y;
            pixels[index + 2] = 0.0f;
            pixels[index + 3] = 1.0f;
        }
    }
    return pixels;
}

std::unique_ptr<RHIBindGroupLayout> CreateSingleUniformLayout(RHIDevice* device,
                                                              uint32_t binding,
                                                              RHIShaderStage visibility,
                                                              const char* debugName)
{
    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({
        binding,
        RHIBindingType::DynamicUniformBuffer,
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

std::unique_ptr<RHIBindGroupLayout> CreateEnvironmentTexturesLayout(RHIDevice* device, const char* debugName)
{
    if (!device)
    {
        return nullptr;
    }

    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({RendererBindings::IrradianceMap, RHIBindingType::TextureCube, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::PrefilterMap, RHIBindingType::TextureCube, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::BRDFLUT, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
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

    if (!EnsureEnvironmentResources())
    {
        TE_LOG_WARN("[Renderer] Environment IBL resources are unavailable; PBR will fall back to direct lighting");
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
    if (m_DefaultWhiteTexture &&
        m_DefaultBlackTexture &&
        m_DefaultNormalTexture &&
        m_DefaultSampler &&
        m_EnvironmentSampler &&
        m_GBufferSampler)
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
        desc.generateMips = true;
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
        desc.generateMips = true;
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
        desc.generateMips = true;
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
        desc.minFilter = RHITextureFilter::LinearMipmapLinear;
        desc.magFilter = RHITextureFilter::Linear;
        desc.addressU = RHITextureAddressMode::Repeat;
        desc.addressV = RHITextureAddressMode::Repeat;
        desc.addressW = RHITextureAddressMode::Repeat;
        desc.enableAnisotropy = true;
        desc.maxAnisotropy = 8.0f;
        desc.debugName = "Default_MipAnisotropic_Sampler";

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

    if (!m_EnvironmentSampler)
    {
        RHISamplerDesc desc;
        desc.minFilter = RHITextureFilter::LinearMipmapLinear;
        desc.magFilter = RHITextureFilter::Linear;
        desc.addressU = RHITextureAddressMode::ClampToEdge;
        desc.addressV = RHITextureAddressMode::ClampToEdge;
        desc.addressW = RHITextureAddressMode::ClampToEdge;
        desc.enableAnisotropy = true;
        desc.maxAnisotropy = 8.0f;
        desc.debugName = "Environment_Cubemap_Sampler";

        auto sampler = m_Device->CreateSampler(desc);
        if (!sampler || !sampler->IsValid())
        {
            TE_LOG_ERROR("[Renderer] Failed to create environment sampler");
            return false;
        }
        m_EnvironmentSampler = std::shared_ptr<RHISampler>(sampler.release());
    }

    return true;
}

bool FRenderResourceManager::EnsureEnvironmentResources()
{
    if (m_EnvironmentIBLResources.EnvironmentMap &&
        m_EnvironmentIBLResources.IrradianceMap &&
        m_EnvironmentIBLResources.PrefilterMap &&
        m_EnvironmentIBLResources.BRDFLUT)
    {
        return true;
    }

    if (!m_Device || !EnsureDefaultTextureResources())
    {
        return false;
    }

    const std::filesystem::path hdrPath =
        std::filesystem::path(TE_PROJECT_ROOT_DIR) / "Content/Textures/HDR/citrus_orchard_road_puresky_4k.hdr";
    if (!std::filesystem::exists(hdrPath))
    {
        TE_LOG_WARN("[Renderer] HDR environment file not found: {}", hdrPath.string());
        return false;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    float* rawPixels = stbi_loadf(hdrPath.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!rawPixels || width <= 0 || height <= 0)
    {
        TE_LOG_WARN("[Renderer] Failed to load HDR environment '{}': {}", hdrPath.string(), stbi_failure_reason());
        if (rawPixels)
        {
            stbi_image_free(rawPixels);
        }
        return false;
    }

    FHDRImage image;
    image.Width = width;
    image.Height = height;
    image.Pixels.assign(rawPixels, rawPixels + static_cast<size_t>(width) * height * 4);
    stbi_image_free(rawPixels);

    TE_LOG_INFO("[Renderer] Building runtime IBL from HDR: {}", hdrPath.string());
    const std::vector<float> environmentPixels = GenerateEnvironmentCubePixels(image, EnvironmentCubeSize);
    const std::vector<float> irradiancePixels = GenerateIrradianceCubePixels(image, IrradianceCubeSize);
    const std::vector<float> brdfPixels = GenerateBRDFLUTPixels(BRDFLUTSize);

    auto createCube = [this](uint32_t size, const std::vector<float>& pixels, const char* debugName)
    {
        RHITextureDesc desc;
        desc.dimension = RHITextureDimension::TextureCube;
        desc.width = size;
        desc.height = size;
        desc.format = RHIFormat::RGBA32_Float;
        desc.initialData = pixels.data();
        desc.generateMips = true;
        desc.srgb = false;
        desc.debugName = debugName;
        auto texture = m_Device->CreateTexture(desc);
        if (!texture || !texture->IsValid())
        {
            TE_LOG_WARN("[Renderer] Failed to create IBL cubemap '{}'", debugName);
            return std::shared_ptr<RHITexture>();
        }
        return std::shared_ptr<RHITexture>(texture.release());
    };

    auto create2D = [this](uint32_t size, const std::vector<float>& pixels, const char* debugName)
    {
        RHITextureDesc desc;
        desc.dimension = RHITextureDimension::Texture2D;
        desc.width = size;
        desc.height = size;
        desc.format = RHIFormat::RGBA32_Float;
        desc.initialData = pixels.data();
        desc.generateMips = true;
        desc.srgb = false;
        desc.debugName = debugName;
        auto texture = m_Device->CreateTexture(desc);
        if (!texture || !texture->IsValid())
        {
            TE_LOG_WARN("[Renderer] Failed to create IBL texture '{}'", debugName);
            return std::shared_ptr<RHITexture>();
        }
        return std::shared_ptr<RHITexture>(texture.release());
    };

    m_EnvironmentIBLResources.EnvironmentMap = createCube(EnvironmentCubeSize, environmentPixels, "IBL_Environment_Cube");
    m_EnvironmentIBLResources.IrradianceMap = createCube(IrradianceCubeSize, irradiancePixels, "IBL_Irradiance_Cube");
    // 当前阶段先使用环境 cubemap 的 mip 链作为初版 specular prefilter，后续可替换为 GGX 预滤波 cubemap。
    m_EnvironmentIBLResources.PrefilterMap = m_EnvironmentIBLResources.EnvironmentMap;
    m_EnvironmentIBLResources.BRDFLUT = create2D(BRDFLUTSize, brdfPixels, "IBL_BRDF_LUT");

    return m_EnvironmentIBLResources.EnvironmentMap &&
           m_EnvironmentIBLResources.IrradianceMap &&
           m_EnvironmentIBLResources.PrefilterMap &&
           m_EnvironmentIBLResources.BRDFLUT;
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
    layouts.push_back({
        RendererBindGroups::Environment,
        CreateEnvironmentTexturesLayout(m_Device, "StaticMeshBasePass_EnvironmentTextures_Layout")
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
    pipelineDesc.depthStencil.depthCompareOp = RendererDepth::CompareOp;
    pipelineDesc.rasterization.cullMode = RHICullMode::Back;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.rendering.colorAttachmentFormats = {m_Device->GetBackBufferColorFormat()};
    pipelineDesc.rendering.depthStencilFormat = m_Device->GetBackBufferDepthFormat();
    pipelineDesc.rendering.colorBlendAttachments.resize(1);
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

const FEnvironmentIBLResources* FRenderResourceManager::GetEnvironmentIBLResources() const
{
    if (!m_EnvironmentIBLResources.IrradianceMap ||
        !m_EnvironmentIBLResources.PrefilterMap ||
        !m_EnvironmentIBLResources.BRDFLUT)
    {
        return nullptr;
    }
    return &m_EnvironmentIBLResources;
}

RHISampler* FRenderResourceManager::GetDefaultSampler() const
{
    return m_DefaultSampler.get();
}

RHISampler* FRenderResourceManager::GetEnvironmentSampler() const
{
    return m_EnvironmentSampler.get();
}

RHISampler* FRenderResourceManager::GetGBufferSampler() const
{
    return m_GBufferSampler.get();
}

} // namespace TE
