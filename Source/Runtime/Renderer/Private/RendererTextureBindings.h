// ToyEngine Renderer Module
// RendererTextureBindings - Forward / Deferred 主路径共享的纹理 BindGroup 上传

#pragma once

#include <memory>

namespace TE {

class RHIBindGroup;
class RHIBindGroupLayout;
class RHICommandBuffer;
class RHIDevice;
class RHISampler;
class RHITexture;
class RHIRenderTarget;

struct FBaseColorTextureBindingState
{
    RHITexture* Texture = nullptr;
    RHISampler* Sampler = nullptr;
    std::unique_ptr<RHIBindGroupLayout> Layout;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

struct FGBufferTextureBindingState
{
    RHITexture* Albedo = nullptr;
    RHITexture* Normal = nullptr;
    RHITexture* WorldPosition = nullptr;
    RHITexture* Depth = nullptr;
    RHISampler* Sampler = nullptr;
    std::unique_ptr<RHIBindGroupLayout> Layout;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

bool UpdateAndBindBaseColorTexture(RHIDevice* device,
                                   RHICommandBuffer* cmdBuf,
                                   FBaseColorTextureBindingState& state,
                                   RHITexture* texture,
                                   RHISampler* sampler);

bool UpdateAndBindGBufferTextures(RHIDevice* device,
                                  RHICommandBuffer* cmdBuf,
                                  FGBufferTextureBindingState& state,
                                  RHIRenderTarget* gbuffer,
                                  RHISampler* sampler);

} // namespace TE
