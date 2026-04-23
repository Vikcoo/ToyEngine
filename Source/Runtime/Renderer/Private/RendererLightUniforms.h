// ToyEngine Renderer Module
// RendererLightUniforms - Forward/Deferred 过渡期共用的光源 uniform 绑定

#pragma once

namespace TE {

class FScene;
class RHICommandBuffer;

void BindSceneLightUniforms(const FScene* scene, RHICommandBuffer* cmdBuf);

} // namespace TE
