// ToyEngine RHI Module
// RHI Sampler 抽象接口 - 封装纹理采样状态

#pragma once

#include "RHITypes.h"

namespace TE {

/// 纹理采样器抽象接口
/// 对应 Vulkan VkSampler / D3D12 Sampler Descriptor / OpenGL Sampler
class RHISampler
{
public:
    virtual ~RHISampler() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;

protected:
    RHISampler() = default;
};

} // namespace TE

