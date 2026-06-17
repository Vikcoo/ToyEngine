// ToyEngine Renderer Module
// RendererPassUniforms - Forward / Deferred 主路径共享的对象常量与 pass 常量上传

#pragma once

#include "Material.h"
#include "Math/MathTypes.h"

#include <memory>

namespace TE {

class RHIBindGroup;
class RHIBindGroupLayout;
class RHIBuffer;
class RHICommandBuffer;
class RHIDevice;
enum class ERenderDebugView : uint8_t;

struct FObjectUniformBindingState
{
    std::unique_ptr<RHIBindGroupLayout> Layout;
    std::unique_ptr<RHIBuffer> UniformBuffer;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

struct FDeferredPassUniformBindingState
{
    std::unique_ptr<RHIBindGroupLayout> Layout;
    std::unique_ptr<RHIBuffer> UniformBuffer;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

struct FMaterialUniformBindingState
{
    std::unique_ptr<RHIBindGroupLayout> Layout;
    std::unique_ptr<RHIBuffer> UniformBuffer;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

struct FSkyUniformBindingState
{
    std::unique_ptr<RHIBindGroupLayout> Layout;
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
                                       ERenderDebugView debugViewMode,
                                       const Vector3& cameraPosition,
                                       const Matrix4& invViewProjection);

bool EnsureMaterialUniformBindingState(RHIDevice* device, FMaterialUniformBindingState& state);
bool UpdateAndBindMaterialUniforms(RHIDevice* device,
                                   RHICommandBuffer* cmdBuf,
                                   FMaterialUniformBindingState& state,
                                   const FMaterial* material,
                                   const Vector3& cameraPosition);

bool EnsureSkyUniformBindingState(RHIDevice* device, FSkyUniformBindingState& state);
bool UpdateAndBindSkyUniforms(RHIDevice* device,
                              RHICommandBuffer* cmdBuf,
                              FSkyUniformBindingState& state,
                              const Matrix4& invViewProjection,
                              const Vector3& cameraPosition);

} // namespace TE
