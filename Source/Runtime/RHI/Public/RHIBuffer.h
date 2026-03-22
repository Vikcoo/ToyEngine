// ToyEngine RHI Module
// RHI Buffer 抽象接口 - 封装 GPU 缓冲区资源

#pragma once

#include "RHITypes.h"

namespace TE {

/// GPU 缓冲区抽象接口
/// 对应 Vulkan VkBuffer / D3D12 ID3D12Resource / OpenGL VBO/IBO/UBO
class RHIBuffer
{
public:
    virtual ~RHIBuffer() = default;

    /// 获取缓冲区大小（字节）
    [[nodiscard]] virtual uint64_t GetSize() const = 0;

    /// 获取缓冲区用途
    [[nodiscard]] virtual RHIBufferUsage GetUsage() const = 0;

protected:
    RHIBuffer() = default;
};

} // namespace TE
