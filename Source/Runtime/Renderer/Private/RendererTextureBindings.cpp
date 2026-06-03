// ToyEngine Renderer Module
// RendererTextureBindings - 主渲染路径纹理 BindGroup 上传

#include "RendererTextureBindings.h"

#include "RendererBindingSlots.h"
#include "RHIBindGroup.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHIRenderTarget.h"
#include "RHISampler.h"
#include "RHITexture.h"
#include "RHITypes.h"

namespace TE {

namespace {

bool RebuildBaseColorBindGroup(RHIDevice* device,
                               FBaseColorTextureBindingState& state,
                               RHITexture* texture,
                               RHISampler* sampler)
{
    if (!device || !texture)
    {
        return false;
    }

    RHIBindGroupDesc bindGroupDesc;
    bindGroupDesc.debugName = "Renderer_BaseColorTexture_BindGroup";
    bindGroupDesc.entries.push_back({
        RendererBindingSlots::BaseColorTexture,
        RHIBindingType::Texture2D,
        nullptr,
        0,
        0,
        texture,
        sampler
    });

    state.BindGroup = device->CreateBindGroup(bindGroupDesc);
    if (!state.BindGroup || !state.BindGroup->IsValid())
    {
        state.BindGroup.reset();
        return false;
    }

    state.Texture = texture;
    state.Sampler = sampler;
    return true;
}

bool RebuildGBufferBindGroup(RHIDevice* device,
                             FGBufferTextureBindingState& state,
                             RHIRenderTarget* gbuffer,
                             RHISampler* sampler)
{
    if (!device || !gbuffer)
    {
        return false;
    }

    auto* albedo = gbuffer->GetColorAttachment(0);
    auto* normal = gbuffer->GetColorAttachment(1);
    auto* worldPosition = gbuffer->GetColorAttachment(2);
    auto* depth = gbuffer->GetDepthStencilAttachment();
    if (!albedo || !normal || !worldPosition || !depth)
    {
        return false;
    }

    RHIBindGroupDesc bindGroupDesc;
    bindGroupDesc.debugName = "Renderer_GBufferTextures_BindGroup";
    bindGroupDesc.entries.push_back({
        RendererBindingSlots::GBufferAlbedo,
        RHIBindingType::Texture2D,
        nullptr,
        0,
        0,
        albedo,
        sampler
    });
    bindGroupDesc.entries.push_back({
        RendererBindingSlots::GBufferNormal,
        RHIBindingType::Texture2D,
        nullptr,
        0,
        0,
        normal,
        sampler
    });
    bindGroupDesc.entries.push_back({
        RendererBindingSlots::GBufferWorldPosition,
        RHIBindingType::Texture2D,
        nullptr,
        0,
        0,
        worldPosition,
        sampler
    });
    bindGroupDesc.entries.push_back({
        RendererBindingSlots::GBufferDepth,
        RHIBindingType::Texture2D,
        nullptr,
        0,
        0,
        depth,
        sampler
    });

    state.BindGroup = device->CreateBindGroup(bindGroupDesc);
    if (!state.BindGroup || !state.BindGroup->IsValid())
    {
        state.BindGroup.reset();
        return false;
    }

    state.Albedo = albedo;
    state.Normal = normal;
    state.WorldPosition = worldPosition;
    state.Depth = depth;
    state.Sampler = sampler;
    return true;
}

} // namespace

bool UpdateAndBindBaseColorTexture(RHIDevice* device,
                                   RHICommandBuffer* cmdBuf,
                                   FBaseColorTextureBindingState& state,
                                   RHITexture* texture,
                                   RHISampler* sampler)
{
    if (!cmdBuf || !texture)
    {
        return false;
    }

    if (!state.BindGroup || state.Texture != texture || state.Sampler != sampler)
    {
        if (!RebuildBaseColorBindGroup(device, state, texture, sampler))
        {
            return false;
        }
    }

    cmdBuf->SetBindGroup(2, state.BindGroup.get());
    return true;
}

bool UpdateAndBindGBufferTextures(RHIDevice* device,
                                  RHICommandBuffer* cmdBuf,
                                  FGBufferTextureBindingState& state,
                                  RHIRenderTarget* gbuffer,
                                  RHISampler* sampler)
{
    if (!cmdBuf || !gbuffer)
    {
        return false;
    }

    auto* albedo = gbuffer->GetColorAttachment(0);
    auto* normal = gbuffer->GetColorAttachment(1);
    auto* worldPosition = gbuffer->GetColorAttachment(2);
    auto* depth = gbuffer->GetDepthStencilAttachment();

    if (!state.BindGroup ||
        state.Albedo != albedo ||
        state.Normal != normal ||
        state.WorldPosition != worldPosition ||
        state.Depth != depth ||
        state.Sampler != sampler)
    {
        if (!RebuildGBufferBindGroup(device, state, gbuffer, sampler))
        {
            return false;
        }
    }

    cmdBuf->SetBindGroup(2, state.BindGroup.get());
    return true;
}

} // namespace TE
