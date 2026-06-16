// ToyEngine Renderer Module
// FDeferredRenderPath - 最小 Deferred 渲染路径

#pragma once

#include "IRenderPath.h"
#include "MeshDrawCommand.h"
#include "MeshPassProcessor.h"
#include "RHIBindGroup.h"
#include "RHIPipeline.h"

#include <memory>
#include <vector>

namespace TE {

class RHIDevice;
class RHIBindGroupLayout;
class RHIPipeline;
class RHIPipelineLayout;
class RHIRenderTarget;
class RHIShader;
struct FLightUniformBindingState;
struct FObjectUniformBindingState;
struct FDeferredPassUniformBindingState;
struct FMaterialTextureBindingState;
struct FMaterialUniformBindingState;
struct FGBufferTextureBindingState;

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
        std::vector<std::unique_ptr<RHIBindGroupLayout>> BindGroupLayouts;
        std::unique_ptr<RHIPipelineLayout> PipelineLayout;
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
    std::unique_ptr<FLightUniformBindingState> m_LightBindingState;
    std::unique_ptr<FObjectUniformBindingState> m_ObjectBindingState;
    std::unique_ptr<FDeferredPassUniformBindingState> m_DeferredPassBindingState;
    std::unique_ptr<FMaterialTextureBindingState> m_MaterialTextureBindingState;
    std::unique_ptr<FMaterialUniformBindingState> m_MaterialBindingState;
    std::unique_ptr<FGBufferTextureBindingState> m_GBufferTextureBindingState;
};

} // namespace TE
