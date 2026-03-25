// ToyEngine Core Module
// 内存分配标签（用于统计/分类）
#pragma once

#include <cstdint>

namespace TE {

enum class MemoryTag : std::uint16_t
{
    Unknown = 0,
    STL,
    Core,
    Platform,
    RHI,
    Renderer,
    Asset,
    Scene,
    Sandbox,

    Count,
};

} // namespace TE

