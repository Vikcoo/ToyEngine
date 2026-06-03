// ToyEngine Renderer Module
// RendererPassUniforms - Forward / Deferred 主路径共享的对象常量与 pass 常量上传

#pragma once

#include "Math/MathTypes.h"

#include <memory>

namespace TE {

class RHIBindGroup;
class RHIBuffer;
class RHICommandBuffer;
class RHIDevice;
enum class ERenderDebugView : uint8_t;

struct FObjectUniformBindingState
{
    std::unique_ptr<RHIBuffer> UniformBuffer;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

struct FDeferredPassUniformBindingState
{
    std::unique_ptr<RHIBuffer> UniformBuffer;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

bool EnsureObjectUniformBindingState(RHIDevice* device, FObjectUniformBindingState& state);
bool UpdateAndBindObjectUniforms(RHIDevice* device,
                                 RHICommandBuffer* cmdBuf,
                                 FObjectUniformBindingState& state,
                                 const Matrix4& mvp,
                                 const Matrix4& model,
                                 const Matrix3& normalMatrix);

bool EnsureDeferredPassUniformBindingState(RHIDevice* device, FDeferredPassUniformBindingState& state);
bool UpdateAndBindDeferredPassUniforms(RHIDevice* device,
                                       RHICommandBuffer* cmdBuf,
                                       FDeferredPassUniformBindingState& state,
                                       bool rtSampleFlipY,
                                       ERenderDebugView debugViewMode);

} // namespace TE
