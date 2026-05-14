// ToyEngine Renderer Module
// FDeferredRenderPath - 最小 Deferred 渲染路径

#pragma once

#include "IRenderPath.h"
#include "MeshDrawCommand.h"
#include "MeshPassProcessor.h"

#include <memory>
#include <vector>

namespace TE {

class RHIDevice;
class RHIPipeline;
class RHIRenderTarget;
class RHIShader;

class FDeferredRenderPath final : public IRenderPath
{
public:
    FDeferredRenderPath();
    ~FDeferredRenderPath() override;

    void Render(const FScene* scene,
                RHIDevice* device,
                RHICommandBuffer* cmdBuf,
                FRenderStats& outStats) override;
    void SetDebugViewMode(ERenderDebugView mode) override { m_DebugViewMode = mode; }

private:
    struct FPreparedStandalonePipeline
    {
        std::unique_ptr<RHIShader> VertexShader;
        std::unique_ptr<RHIShader> FragmentShader;
        std::unique_ptr<RHIPipeline> Pipeline;
    };

    [[nodiscard]] bool EnsureResources(RHIDevice* device, uint32_t width, uint32_t height);
    [[nodiscard]] bool EnsurePipelines(RHIDevice* device);
    [[nodiscard]] bool EnsureGBuffer(RHIDevice* device, uint32_t width, uint32_t height);
    [[nodiscard]] bool BuildGBufferPipeline(RHIDevice* device);
    [[nodiscard]] bool BuildLightingPipeline(RHIDevice* device);

    void SortDrawCommands(std::vector<FMeshDrawCommand>& commands) const;
    void SubmitGBufferPass(const std::vector<FMeshDrawCommand>& commands,
                           const FScene* scene,
                           RHIDevice* device,
                           RHICommandBuffer* cmdBuf,
                           FRenderStats& outStats) const;
    void SubmitLightingPass(const FScene* scene,
                            RHIDevice* device,
                            RHICommandBuffer* cmdBuf,
                            FRenderStats& outStats) const;

    FMeshPassProcessor m_GBufferPassProcessor;
    FPreparedStandalonePipeline m_GBufferPipeline;
    FPreparedStandalonePipeline m_LightingPipeline;
    std::unique_ptr<RHIRenderTarget> m_GBuffer;
    uint32_t m_GBufferWidth = 0;
    uint32_t m_GBufferHeight = 0;
    ERenderDebugView m_DebugViewMode = ERenderDebugView::Lit;
};

} // namespace TE
