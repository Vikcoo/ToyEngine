// ToyEngine Renderer Module
// Renderer 瞬态 Uniform 绑定实现

#include "RendererTransientUniforms.h"

#include "RHIBindGroup.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"

#include <array>
#include <limits>

namespace TE {

bool AllocateAndBindTransientUniform(RHIDevice* device,
                                     RHICommandBuffer* cmdBuf,
                                     FTransientUniformBindingState& state,
                                     const void* data,
                                     const uint64_t size,
                                     const uint32_t groupIndex,
                                     const uint32_t binding,
                                     const RHIShaderStage visibility,
                                     const char* debugName)
{
    if (!device || !cmdBuf || !data || size == 0)
    {
        return false;
    }

    RHITransientUniformAllocation allocation;
    if (!device->AllocateTransientUniform(data, size, allocation) || !allocation.buffer)
    {
        return false;
    }

    if (!state.Layout)
    {
        RHIBindGroupLayoutDesc layoutDesc;
        layoutDesc.debugName = debugName;
        layoutDesc.entries.push_back({binding, RHIBindingType::DynamicUniformBuffer, visibility});
        state.Layout = device->CreateBindGroupLayout(layoutDesc);
        if (!state.Layout || !state.Layout->IsValid())
        {
            return false;
        }
    }

    if (!state.BindGroup || state.Buffer != allocation.buffer || state.Range != size)
    {
        RHIBindGroupDesc bindGroupDesc;
        bindGroupDesc.layout = state.Layout.get();
        bindGroupDesc.debugName = debugName;
        bindGroupDesc.entries.push_back({
            binding,
            RHIBindingType::DynamicUniformBuffer,
            allocation.buffer,
            0,
            size,
            nullptr,
            nullptr
        });

        state.BindGroup = device->CreateBindGroup(bindGroupDesc);
        if (!state.BindGroup || !state.BindGroup->IsValid())
        {
            state.BindGroup.reset();
            return false;
        }

        state.Buffer = allocation.buffer;
        state.Range = size;
    }

    if (allocation.offset > std::numeric_limits<uint32_t>::max())
    {
        return false;
    }

    const std::array<uint32_t, 1> dynamicOffsets = {static_cast<uint32_t>(allocation.offset)};
    cmdBuf->SetBindGroup(groupIndex, state.BindGroup.get(), dynamicOffsets);
    return true;
}

} // namespace TE
