// ToyEngine RenderCore Module
// FMeshDrawCommand - 单条网格绘制命令

#pragma once

#include "Math/MathTypes.h"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace TE {

class RHIBuffer;
class StaticMesh;

enum class EMeshPassType : uint8_t
{
    BasePass = 0,
    GBuffer = 1,
    Lighting = 2,
};

enum class EMaterialDomain : uint8_t
{
    Opaque = 0,
};

enum class EVertexFactoryType : uint8_t
{
    StaticMesh = 0,
};

struct FPipelineKey
{
    EMeshPassType Pass = EMeshPassType::BasePass;
    EMaterialDomain MaterialDomain = EMaterialDomain::Opaque;
    EVertexFactoryType VertexFactory = EVertexFactoryType::StaticMesh;

    [[nodiscard]] static constexpr FPipelineKey StaticMeshBasePass()
    {
        return {EMeshPassType::BasePass, EMaterialDomain::Opaque, EVertexFactoryType::StaticMesh};
    }

    [[nodiscard]] constexpr bool operator==(const FPipelineKey& other) const
    {
        return Pass == other.Pass &&
               MaterialDomain == other.MaterialDomain &&
               VertexFactory == other.VertexFactory;
    }

    [[nodiscard]] constexpr bool operator!=(const FPipelineKey& other) const
    {
        return !(*this == other);
    }
};

struct FPipelineKeyHash
{
    [[nodiscard]] std::size_t operator()(const FPipelineKey& key) const
    {
        const auto pass = static_cast<std::size_t>(key.Pass);
        const auto domain = static_cast<std::size_t>(key.MaterialDomain);
        const auto vertexFactory = static_cast<std::size_t>(key.VertexFactory);
        return pass ^ (domain << 8) ^ (vertexFactory << 16);
    }
};

struct FMeshDrawCommand
{
    FPipelineKey PipelineKey = FPipelineKey::StaticMeshBasePass();
    RHIBuffer* VertexBuffer = nullptr;
    RHIBuffer* IndexBuffer = nullptr;
    // 保留资产引用用于渲染侧解析材质贴图（Proxy 不直接持有 GPU 纹理对象）。
    const StaticMesh* StaticMeshAsset = nullptr;
    uint32_t FirstIndex = 0;
    uint32_t IndexCount = 0;
    uint32_t MaterialIndex = 0;
    Matrix4 WorldMatrix = Matrix4::Identity;
};

} // namespace TE
