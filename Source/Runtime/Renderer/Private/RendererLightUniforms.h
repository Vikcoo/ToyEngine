// ToyEngine Renderer Module
// RendererLightUniforms - Forward/Deferred 共用的 LightBlock UBO 上传

#pragma once

#include "RHIBindGroup.h"
#include "RHIBuffer.h"
#include "RendererTransientUniforms.h"

#include <memory>

namespace TE {

class FScene;
class RHIBindGroupLayout;
class RHICommandBuffer;
class RHIDevice;

struct FLightUniformBindingState : FTransientUniformBindingState {};

bool UpdateAndBindSceneLightUniforms(const FScene* scene,
                                     RHIDevice* device,
                                     RHICommandBuffer* cmdBuf,
                                     FLightUniformBindingState& state);

} // namespace TE
