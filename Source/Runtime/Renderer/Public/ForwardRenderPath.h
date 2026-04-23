// ToyEngine Renderer Module
// FForwardRenderPath - 当前 Forward BasePass 渲染路径

#pragma once

#include "IRenderPath.h"
#include "MeshDrawCommand.h"
#include "MeshPassProcessor.h"

#include <vector>

namespace TE {

class RHIBuffer;
class RHIDevice;
class RHIPipeline;

class FForwardRenderPath final : public IRenderPath
{
public:
    FForwardRenderPath();
    ~FForwardRenderPath() override = default;

    void Render(const FScene* scene,
                RHIDevice* device,
                RHICommandBuffer* cmdBuf,
                FRenderStats& outStats) override;

private:
    void SortDrawCommands(std::vector<FMeshDrawCommand>& commands) const;
    void SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                            const FScene* scene,
                            RHIDevice* device,
                            RHICommandBuffer* cmdBuf,
                            FRenderStats& outStats) const;

    FMeshPassProcessor m_BasePassProcessor;
};

} // namespace TE
