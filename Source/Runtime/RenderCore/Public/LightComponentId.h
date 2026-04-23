// ToyEngine RenderCore Module
// FLightComponentId - Light 在渲染同步边界的稳定身份

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace TE {

struct FLightComponentId
{
    uint32_t Value = 0;

    [[nodiscard]] bool IsValid() const { return Value != 0; }

    friend bool operator==(const FLightComponentId& lhs, const FLightComponentId& rhs)
    {
        return lhs.Value == rhs.Value;
    }
};

struct FLightComponentIdHash
{
    std::size_t operator()(const FLightComponentId& lightComponentId) const noexcept
    {
        return std::hash<uint32_t>{}(lightComponentId.Value);
    }
};

} // namespace TE
