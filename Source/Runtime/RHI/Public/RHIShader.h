// ToyEngine RHI Module
// RHI Shader 抽象接口 - 封装着色器模块

#pragma once

#include "RHITypes.h"

namespace TE {

/// 着色器模块抽象接口
/// 对应 Vulkan VkShaderModule / D3D12 ID3DBlob / OpenGL glCreateShader
class RHIShader
{
public:
    virtual ~RHIShader() = default;

    /// 获取着色器阶段
    [[nodiscard]] virtual RHIShaderStage GetStage() const = 0;

    /// 着色器是否有效（编译成功）
    [[nodiscard]] virtual bool IsValid() const = 0;

protected:
    RHIShader() = default;
};

} // namespace TE
