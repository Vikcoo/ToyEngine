// ToyEngine Renderer Module
// IRenderPath - 渲染路径策略边界

#pragma once

#include "RenderPathTypes.h"

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
    virtual void SetDebugViewMode(ERenderDebugView mode) { (void)mode; }
};

} // namespace TE
