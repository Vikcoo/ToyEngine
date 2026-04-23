// ToyEngine Renderer Module
// IRenderPath - 渲染路径策略边界

#pragma once

namespace TE {

class FScene;
class RHIDevice;
class RHICommandBuffer;
struct FRenderStats;

class IRenderPath
{
public:
    virtual ~IRenderPath() = default;

    virtual void Render(const FScene* scene,
                        RHIDevice* device,
                        RHICommandBuffer* cmdBuf,
                        FRenderStats& outStats) = 0;
};

} // namespace TE
