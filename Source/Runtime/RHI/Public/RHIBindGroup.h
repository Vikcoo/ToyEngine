// ToyEngine RHI Module
// RHI BindGroup 抽象接口 - 封装一组 Shader 资源绑定
// 对应 Vulkan VkDescriptorSet / D3D12 Descriptor Table / WebGPU GPUBindGroup

#pragma once

#include "RHITypes.h"

namespace TE {

/// 资源绑定组抽象接口
/// 将 UBO、纹理、采样器等资源打包为一组，供 Pipeline 使用。
class RHIBindGroup
{
public:
    virtual ~RHIBindGroup() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;

protected:
    RHIBindGroup() = default;
};

} // namespace TE
