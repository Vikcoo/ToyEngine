// ToyEngine Renderer Module
// RendererLightUniforms - Forward/Deferred 共用的 LightBlock UBO 上传

#pragma once

#include "RHIBindGroup.h"
#include "RHIBuffer.h"

#include <memory>

namespace TE {

class FScene;
class RHICommandBuffer;
class RHIDevice;

struct FLightUniformBindingState
{
    std::unique_ptr<RHIBuffer> UniformBuffer;
    std::unique_ptr<RHIBindGroup> BindGroup;
};

bool EnsureLightUniformBindingState(RHIDevice* device, FLightUniformBindingState& state);
bool UpdateAndBindSceneLightUniforms(const FScene* scene,
                                     RHIDevice* device,
                                     RHICommandBuffer* cmdBuf,
                                     FLightUniformBindingState& state);

} // namespace TE
