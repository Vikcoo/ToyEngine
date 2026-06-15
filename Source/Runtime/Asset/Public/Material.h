// ToyEngine Asset Module
// Material - 最小材质资产描述

#pragma once

#include "Math/MathTypes.h"

#include <cstdint>
#include <string>
#include <utility>

namespace TE {

enum class EMaterialTextureSemantic : uint8_t
{
    BaseColor,
    MetallicRoughness,
    Metallic,
    Roughness,
    Normal,
    AmbientOcclusion,
    Emissive,
};

enum class ETextureColorSpace : uint8_t
{
    Linear,
    SRGB,
};

struct FMaterialTextureSlot
{
    EMaterialTextureSemantic Semantic = EMaterialTextureSemantic::BaseColor;
    ETextureColorSpace ColorSpace = ETextureColorSpace::Linear;
    std::string TexturePath;

    [[nodiscard]] bool HasTexture() const { return !TexturePath.empty(); }
};

/// 最小 PBR 材质描述。
///
/// 当前 Renderer 仍只消费 BaseColor 贴图；其它字段先作为资产与资源语义基础，
/// 供后续 Forward PBR / Deferred PBR 阶段接入。
struct FMaterial
{
    std::string Name;

    Vector3 BaseColorFactor = Vector3(1.0f, 1.0f, 1.0f);
    float MetallicFactor = 0.0f;
    float RoughnessFactor = 1.0f;
    float AmbientOcclusionFactor = 1.0f;
    Vector3 EmissiveFactor = Vector3(0.0f, 0.0f, 0.0f);
    float EmissiveStrength = 1.0f;

    FMaterialTextureSlot BaseColorTexture{
        EMaterialTextureSemantic::BaseColor,
        ETextureColorSpace::SRGB,
        {}
    };
    FMaterialTextureSlot MetallicRoughnessTexture{
        EMaterialTextureSemantic::MetallicRoughness,
        ETextureColorSpace::Linear,
        {}
    };
    FMaterialTextureSlot MetallicTexture{
        EMaterialTextureSemantic::Metallic,
        ETextureColorSpace::Linear,
        {}
    };
    FMaterialTextureSlot RoughnessTexture{
        EMaterialTextureSemantic::Roughness,
        ETextureColorSpace::Linear,
        {}
    };
    FMaterialTextureSlot NormalTexture{
        EMaterialTextureSemantic::Normal,
        ETextureColorSpace::Linear,
        {}
    };
    FMaterialTextureSlot AmbientOcclusionTexture{
        EMaterialTextureSemantic::AmbientOcclusion,
        ETextureColorSpace::Linear,
        {}
    };
    FMaterialTextureSlot EmissiveTexture{
        EMaterialTextureSemantic::Emissive,
        ETextureColorSpace::SRGB,
        {}
    };

    [[nodiscard]] bool HasBaseColorTexture() const { return BaseColorTexture.HasTexture(); }
    [[nodiscard]] bool HasMetallicRoughnessTexture() const { return MetallicRoughnessTexture.HasTexture(); }
    [[nodiscard]] bool HasMetallicTexture() const { return MetallicTexture.HasTexture(); }
    [[nodiscard]] bool HasRoughnessTexture() const { return RoughnessTexture.HasTexture(); }
    [[nodiscard]] bool HasNormalTexture() const { return NormalTexture.HasTexture(); }
    [[nodiscard]] bool HasAmbientOcclusionTexture() const { return AmbientOcclusionTexture.HasTexture(); }
    [[nodiscard]] bool HasEmissiveTexture() const { return EmissiveTexture.HasTexture(); }

    void SetBaseColorTexturePath(std::string path) { BaseColorTexture.TexturePath = std::move(path); }
    [[nodiscard]] const std::string& GetBaseColorTexturePath() const { return BaseColorTexture.TexturePath; }
};

} // namespace TE
