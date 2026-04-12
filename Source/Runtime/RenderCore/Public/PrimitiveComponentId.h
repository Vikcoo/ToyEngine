// ToyEngine RenderCore Module
// PrimitiveComponentId - Primitive 在渲染同步边界的稳定身份

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

namespace TE {

struct FPrimitiveComponentId
{
    static constexpr uint32_t InvalidValue = 0;

    uint32_t Value = InvalidValue;

    [[nodiscard]] bool IsValid() const { return Value != InvalidValue; }

    friend bool operator==(const FPrimitiveComponentId& lhs, const FPrimitiveComponentId& rhs)
    {
        return lhs.Value == rhs.Value;
    }
};

struct FPrimitiveComponentIdHash
{
    std::size_t operator()(const FPrimitiveComponentId& primitiveComponentId) const noexcept
    {
        return std::hash<uint32_t>{}(primitiveComponentId.Value);
    }
};

} // namespace TE
