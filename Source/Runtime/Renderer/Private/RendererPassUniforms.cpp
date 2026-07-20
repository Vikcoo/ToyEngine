// ToyEngine Renderer Module
// RendererPassUniforms - 主渲染路径共享常量块上传

#include "RendererPassUniforms.h"

#include "RendererBindingSlots.h"
#include "RenderPathTypes.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"

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

} // namespace

bool UpdateAndBindObjectUniforms(RHIDevice* device,
                                 RHICommandBuffer* cmdBuf,
                                 FObjectUniformBindingState& state,
                                 const Matrix4& mvp,
                                 const Matrix4& model,
                                 const Matrix3& normalMatrix)
{
    if (!cmdBuf)
    {
        return false;
    }

    FObjectBlockCPU objectBlock{};
    objectBlock.MVP = mvp;
    objectBlock.Model = model;
    objectBlock.NormalMatrix = ExpandNormalMatrixToMatrix4(normalMatrix);

    return AllocateAndBindTransientUniform(device,
                                           cmdBuf,
                                           state,
                                           &objectBlock,
                                           sizeof(objectBlock),
                                           RendererBindGroups::PassBlock,
                                           RendererBindings::PassBlock,
                                           RHIShaderStage::Vertex,
                                           "Renderer_ObjectBlock_DynamicBindGroup");
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
    if (!cmdBuf)
    {
        return false;
    }

    FDeferredPassBlockCPU passBlock{};
    passBlock.RTSampleFlipY = rtSampleFlipY ? 1 : 0;
    passBlock.DebugViewMode = static_cast<int32_t>(debugViewMode);
    passBlock.NDCDepthZeroToOne = ndcDepthZeroToOne ? 1 : 0;
    passBlock.CameraPosition_Pad = Vector4(cameraPosition, 0.0f);
    passBlock.InvViewProjection = invViewProjection;

    return AllocateAndBindTransientUniform(device,
                                           cmdBuf,
                                           state,
                                           &passBlock,
                                           sizeof(passBlock),
                                           RendererBindGroups::PassBlock,
                                           RendererBindings::PassBlock,
                                           RHIShaderStage::Fragment,
                                           "Renderer_DeferredPassBlock_DynamicBindGroup");
}

bool UpdateAndBindMaterialUniforms(RHIDevice* device,
                                   RHICommandBuffer* cmdBuf,
                                   FMaterialUniformBindingState& state,
                                   const FMaterial* material,
                                   const Vector3& cameraPosition)
{
    if (!cmdBuf)
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

    return AllocateAndBindTransientUniform(device,
                                           cmdBuf,
                                           state,
                                           &materialBlock,
                                           sizeof(materialBlock),
                                           RendererBindGroups::MaterialBlock,
                                           RendererBindings::MaterialBlock,
                                           RHIShaderStage::Fragment,
                                           "Renderer_MaterialBlock_DynamicBindGroup");
}

bool UpdateAndBindSkyUniforms(RHIDevice* device,
                              RHICommandBuffer* cmdBuf,
                              FSkyUniformBindingState& state,
                              const Matrix4& invViewProjection,
                              const Vector3& cameraPosition)
{
    if (!cmdBuf)
    {
        return false;
    }

    FSkyBlockCPU skyBlock{};
    skyBlock.InvViewProjection = invViewProjection;
    skyBlock.CameraPosition_Pad = Vector4(cameraPosition, 0.0f);

    return AllocateAndBindTransientUniform(device,
                                           cmdBuf,
                                           state,
                                           &skyBlock,
                                           sizeof(skyBlock),
                                           RendererBindGroups::PassBlock,
                                           RendererBindings::PassBlock,
                                           RHIShaderStage::Fragment,
                                           "Renderer_SkyBlock_DynamicBindGroup");
}

} // namespace TE
