// ToyEngine Renderer Module
// Renderer 瞬态 Uniform 绑定 - 统一帧内常量快照与 dynamic offset 提交

#pragma once

#include "RHITypes.h"

#include <memory>

namespace TE {

class RHIBindGroup;
class RHIBindGroupLayout;
class RHIBuffer;
class RHICommandBuffer;
class RHIDevice;

struct FTransientUniformBindingState
{
    std::unique_ptr<RHIBindGroupLayout> Layout;
    std::unique_ptr<RHIBindGroup> BindGroup;
    RHIBuffer* Buffer = nullptr;
    uint64_t Range = 0;
};

[[nodiscard]] bool AllocateAndBindTransientUniform(RHIDevice* device,
                                                   RHICommandBuffer* cmdBuf,
                                                   FTransientUniformBindingState& state,
                                                   const void* data,
                                                   uint64_t size,
                                                   uint32_t groupIndex,
                                                   uint32_t binding,
                                                   RHIShaderStage visibility,
                                                   const char* debugName);

} // namespace TE
