// ToyEngine RHI Module
// RHI BindGroup 抽象接口 - 封装一组 Shader 资源绑定
// 对应 Vulkan VkDescriptorSet / D3D12 Descriptor Table / WebGPU GPUBindGroup

#pragma once

#include "RHITypes.h"

namespace TE {

/// BindGroup 布局抽象接口。
/// 描述一组资源绑定的接口形状，不持有具体资源。
class RHIBindGroupLayout
{
public:
    virtual ~RHIBindGroupLayout() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;

protected:
    RHIBindGroupLayout() = default;
};

/// 资源绑定组抽象接口。
/// 将 UBO、纹理、采样器等具体资源打包为一组，必须匹配某个 BindGroupLayout。
class RHIBindGroup
{
public:
    virtual ~RHIBindGroup() = default;

    [[nodiscard]] virtual bool IsValid() const = 0;

protected:
    RHIBindGroup() = default;
};

} // namespace TE
