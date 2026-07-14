// ToyEngine Renderer Module
// RendererPassUniforms - 主渲染路径共享常量块上传

#include "RendererPassUniforms.h"

#include "RendererBindingSlots.h"
#include "RenderPathTypes.h"
#include "RHIBindGroup.h"
#include "RHIBuffer.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHITypes.h"

namespace TE {

namespace {

struct alignas(16) FObjectBlockCPU
{
    Matrix4 MVP;
    Matrix4 Model;
    Matrix4 NormalMatrix;
};

struct alignas(16) FDeferredPassBlockCPU
{
    int32_t RTSampleFlipY = 0;
    int32_t DebugViewMode = 0;
    int32_t NDCDepthZeroToOne = 1;
    int32_t Reserved1 = 0;
    Vector4 CameraPosition_Pad;
    Matrix4 InvViewProjection;
};

struct alignas(16) FMaterialBlockCPU
{
    Vector4 BaseColorFactor_Metallic;
    Vector4 RoughnessAOEmissiveStrength_Pad;
    Vector4 EmissiveFactor_Pad;
    Vector4 CameraPosition_Pad;
};

struct alignas(16) FSkyBlockCPU
{
    Matrix4 InvViewProjection;
    Vector4 CameraPosition_Pad;
};

static_assert(sizeof(FObjectBlockCPU) % 16 == 0);
static_assert(sizeof(FDeferredPassBlockCPU) % 16 == 0);
static_assert(sizeof(FMaterialBlockCPU) % 16 == 0);
static_assert(sizeof(FSkyBlockCPU) % 16 == 0);

Matrix4 ExpandNormalMatrixToMatrix4(const Matrix3& normalMatrix)
{
    Matrix4 expanded(1.0f);
    for (int column = 0; column < 3; ++column)
    {
        for (int row = 0; row < 3; ++row)
        {
            expanded(column, row) = normalMatrix(column, row);
        }
    }

    expanded(0, 3) = 0.0f;
    expanded(1, 3) = 0.0f;
    expanded(2, 3) = 0.0f;
    expanded(3, 0) = 0.0f;
    expanded(3, 1) = 0.0f;
    expanded(3, 2) = 0.0f;
    expanded(3, 3) = 1.0f;
    return expanded;
}

template <typename TBindingState>
bool CreateUniformBindingState(RHIDevice* device,
                               TBindingState& state,
                               uint64_t bufferSize,
                               uint32_t binding,
                               const char* bufferDebugName,
                               const char* bindGroupDebugName,
                               RHIShaderStage visibility)
{
    if (!device)
    {
        return false;
    }

    RHIBindGroupLayoutDesc layoutDesc;
    layoutDesc.debugName = bindGroupDebugName;
    layoutDesc.entries.push_back({
        binding,
        RHIBindingType::UniformBuffer,
        visibility
    });
    state.Layout = device->CreateBindGroupLayout(layoutDesc);
    if (!state.Layout || !state.Layout->IsValid())
    {
        return false;
    }

    RHIBufferDesc bufferDesc;
    bufferDesc.size = bufferSize;
    bufferDesc.usage = RHIBufferUsage::Uniform;
    bufferDesc.debugName = bufferDebugName;
    state.UniformBuffer = device->CreateBuffer(bufferDesc);
    if (!state.UniformBuffer)
    {
        state.UniformBuffer.reset();
        return false;
    }

    RHIBindGroupDesc bindGroupDesc;
    bindGroupDesc.layout = state.Layout.get();
    bindGroupDesc.debugName = bindGroupDebugName;
    bindGroupDesc.entries.push_back({
        binding,
        RHIBindingType::UniformBuffer,
        state.UniformBuffer.get(),
        0,
        bufferSize,
        nullptr,
        nullptr
    });

    state.BindGroup = device->CreateBindGroup(bindGroupDesc);
    if (!state.BindGroup || !state.BindGroup->IsValid())
    {
        state.UniformBuffer.reset();
        state.BindGroup.reset();
        return false;
    }

    return true;
}

} // namespace

bool EnsureObjectUniformBindingState(RHIDevice* device, FObjectUniformBindingState& state)
{
    if (state.Layout &&
        state.UniformBuffer &&
        state.BindGroup && state.BindGroup->IsValid())
    {
        return true;
    }

    return CreateUniformBindingState(device,
                                     state,
                                     sizeof(FObjectBlockCPU),
                                     RendererBindings::PassBlock,
                                     "Renderer_ObjectBlock_UBO",
                                     "Renderer_ObjectBlock_BindGroup",
                                     RHIShaderStage::Vertex);
}

bool UpdateAndBindObjectUniforms(RHIDevice* device,
                                 RHICommandBuffer* cmdBuf,
                                 FObjectUniformBindingState& state,
                                 const Matrix4& mvp,
                                 const Matrix4& model,
                                 const Matrix3& normalMatrix)
{
    if (!cmdBuf || !EnsureObjectUniformBindingState(device, state))
    {
        return false;
    }

    FObjectBlockCPU objectBlock{};
    objectBlock.MVP = mvp;
    objectBlock.Model = model;
    objectBlock.NormalMatrix = ExpandNormalMatrixToMatrix4(normalMatrix);

    if (!state.UniformBuffer->UpdateData(&objectBlock, sizeof(objectBlock)))
    {
        return false;
    }

    cmdBuf->SetBindGroup(RendererBindGroups::PassBlock, state.BindGroup.get());
    return true;
}

bool EnsureDeferredPassUniformBindingState(RHIDevice* device, FDeferredPassUniformBindingState& state)
{
    if (state.Layout &&
        state.UniformBuffer &&
        state.BindGroup && state.BindGroup->IsValid())
    {
        return true;
    }

    return CreateUniformBindingState(device,
                                     state,
                                     sizeof(FDeferredPassBlockCPU),
                                     RendererBindings::PassBlock,
                                     "Renderer_DeferredPassBlock_UBO",
                                     "Renderer_DeferredPassBlock_BindGroup",
                                     RHIShaderStage::Fragment);
}

bool UpdateAndBindDeferredPassUniforms(RHIDevice* device,
                                       RHICommandBuffer* cmdBuf,
                                       FDeferredPassUniformBindingState& state,
                                       bool rtSampleFlipY,
                                       bool ndcDepthZeroToOne,
                                       ERenderDebugView debugViewMode,
                                       const Vector3& cameraPosition,
                                       const Matrix4& invViewProjection)
{
    if (!cmdBuf || !EnsureDeferredPassUniformBindingState(device, state))
    {
        return false;
    }

    FDeferredPassBlockCPU passBlock{};
    passBlock.RTSampleFlipY = rtSampleFlipY ? 1 : 0;
    passBlock.DebugViewMode = static_cast<int32_t>(debugViewMode);
    passBlock.NDCDepthZeroToOne = ndcDepthZeroToOne ? 1 : 0;
    passBlock.CameraPosition_Pad = Vector4(cameraPosition, 0.0f);
    passBlock.InvViewProjection = invViewProjection;

    if (!state.UniformBuffer->UpdateData(&passBlock, sizeof(passBlock)))
    {
        return false;
    }

    cmdBuf->SetBindGroup(RendererBindGroups::PassBlock, state.BindGroup.get());
    return true;
}

bool EnsureMaterialUniformBindingState(RHIDevice* device, FMaterialUniformBindingState& state)
{
    if (state.Layout &&
        state.UniformBuffer &&
        state.BindGroup && state.BindGroup->IsValid())
    {
        return true;
    }

    return CreateUniformBindingState(device,
                                     state,
                                     sizeof(FMaterialBlockCPU),
                                     RendererBindings::MaterialBlock,
                                     "Renderer_MaterialBlock_UBO",
                                     "Renderer_MaterialBlock_BindGroup",
                                     RHIShaderStage::Fragment);
}

bool UpdateAndBindMaterialUniforms(RHIDevice* device,
                                   RHICommandBuffer* cmdBuf,
                                   FMaterialUniformBindingState& state,
                                   const FMaterial* material,
                                   const Vector3& cameraPosition)
{
    if (!cmdBuf || !EnsureMaterialUniformBindingState(device, state))
    {
        return false;
    }

    FMaterial defaultMaterial;
    const FMaterial& sourceMaterial = material ? *material : defaultMaterial;

    FMaterialBlockCPU materialBlock{};
    materialBlock.BaseColorFactor_Metallic = Vector4(sourceMaterial.BaseColorFactor, sourceMaterial.MetallicFactor);
    materialBlock.RoughnessAOEmissiveStrength_Pad = Vector4(sourceMaterial.RoughnessFactor,
                                                            sourceMaterial.AmbientOcclusionFactor,
                                                            sourceMaterial.EmissiveStrength,
                                                            0.0f);
    materialBlock.EmissiveFactor_Pad = Vector4(sourceMaterial.EmissiveFactor, 0.0f);
    materialBlock.CameraPosition_Pad = Vector4(cameraPosition, 0.0f);

    if (!state.UniformBuffer->UpdateData(&materialBlock, sizeof(materialBlock)))
    {
        return false;
    }

    cmdBuf->SetBindGroup(RendererBindGroups::MaterialBlock, state.BindGroup.get());
    return true;
}

bool EnsureSkyUniformBindingState(RHIDevice* device, FSkyUniformBindingState& state)
{
    if (state.Layout &&
        state.UniformBuffer &&
        state.BindGroup && state.BindGroup->IsValid())
    {
        return true;
    }

    return CreateUniformBindingState(device,
                                     state,
                                     sizeof(FSkyBlockCPU),
                                     RendererBindings::PassBlock,
                                     "Renderer_SkyBlock_UBO",
                                     "Renderer_SkyBlock_BindGroup",
                                     RHIShaderStage::Fragment);
}

bool UpdateAndBindSkyUniforms(RHIDevice* device,
                              RHICommandBuffer* cmdBuf,
                              FSkyUniformBindingState& state,
                              const Matrix4& invViewProjection,
                              const Vector3& cameraPosition)
{
    if (!cmdBuf || !EnsureSkyUniformBindingState(device, state))
    {
        return false;
    }

    FSkyBlockCPU skyBlock{};
    skyBlock.InvViewProjection = invViewProjection;
    skyBlock.CameraPosition_Pad = Vector4(cameraPosition, 0.0f);

    if (!state.UniformBuffer->UpdateData(&skyBlock, sizeof(skyBlock)))
    {
        return false;
    }

    cmdBuf->SetBindGroup(RendererBindGroups::PassBlock, state.BindGroup.get());
    return true;
}

} // namespace TE
