// ToyEngine Renderer Module
// 渲染路径与调试视图公共枚举

#pragma once

#include <cstdint>

namespace TE {

enum class ERenderPathType : uint8_t
{
    Forward = 0,
    Deferred = 1,
};

enum class ERenderDebugView : uint8_t
{
    Lit = 0,
    Albedo = 1,
    Normal = 2,
    WorldPosition = 3,
    Depth = 4,
};

} // namespace TE
