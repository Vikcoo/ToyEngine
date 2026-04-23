// ToyEngine Renderer Module
// FRenderStats - 单帧渲染统计

#pragma once

#include <cstdint>

namespace TE {

struct FRenderStats
{
    uint32_t DrawCallCount = 0;
    uint32_t PipelineBindCount = 0;
    uint32_t VBOBindCount = 0;
    uint32_t IBOBindCount = 0;
};

} // namespace TE
