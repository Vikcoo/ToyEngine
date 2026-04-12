// ToyEngine RenderCore Module
// FMeshDrawCommand - 单条网格绘制命令

#pragma once

#include "Math/MathTypes.h"

#include <cstdint>

namespace TE {

class RHIBuffer;
class StaticMesh;

enum class EMeshPipelineKey : uint8_t
{
    StaticMeshLit = 0,
};

struct FMeshDrawCommand
{
    EMeshPipelineKey PipelineKey = EMeshPipelineKey::StaticMeshLit;
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
