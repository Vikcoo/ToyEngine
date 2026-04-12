// ToyEngine RenderCore Module
// FMeshDrawCommand - 单条网格绘制命令

#pragma once

#include "Math/MathTypes.h"

#include <cstdint>

namespace TE {

class RHIBuffer;

enum class EMeshPipelineKey : uint8_t
{
    StaticMeshLit = 0,
};

struct FMeshDrawCommand
{
    EMeshPipelineKey PipelineKey = EMeshPipelineKey::StaticMeshLit;
    RHIBuffer* VertexBuffer = nullptr;
    RHIBuffer* IndexBuffer = nullptr;
    uint32_t FirstIndex = 0;
    uint32_t IndexCount = 0;
    uint32_t MaterialIndex = 0;
    Matrix4 WorldMatrix = Matrix4::Identity;
};

} // namespace TE
