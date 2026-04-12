// ToyEngine RHI Module
// RHI Texture 抽象接口 - 封装 GPU 纹理资源

#pragma once

#include "RHITypes.h"

namespace TE {

/// GPU 纹理抽象接口
/// 对应 Vulkan VkImage / D3D12 ID3D12Resource(Texture) / OpenGL Texture
class RHITexture
{
public:
    virtual ~RHITexture() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;
    [[nodiscard]] virtual uint32_t GetWidth() const = 0;
    [[nodiscard]] virtual uint32_t GetHeight() const = 0;
    [[nodiscard]] virtual RHIFormat GetFormat() const = 0;

protected:
    RHITexture() = default;
};

} // namespace TE

