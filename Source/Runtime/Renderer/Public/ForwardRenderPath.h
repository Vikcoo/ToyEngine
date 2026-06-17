// ToyEngine Renderer Module
// FForwardRenderPath - 当前 Forward BasePass 渲染路径

#pragma once

#include "IRenderPath.h"
#include "MeshDrawCommand.h"
#include "MeshPassProcessor.h"

#include <memory>
#include <vector>

namespace TE {

class RHIBuffer;
class RHIBindGroupLayout;
class RHIDevice;
class RHIPipeline;
class RHIPipelineLayout;
class RHIShader;
struct FLightUniformBindingState;
struct FObjectUniformBindingState;
struct FMaterialTextureBindingState;
struct FMaterialUniformBindingState;
struct FEnvironmentTextureBindingState;
struct FSkyUniformBindingState;

class FForwardRenderPath final : public IRenderPath
{
public:
    FForwardRenderPath();
    ~FForwardRenderPath() override;

    void Render(const FScene* scene,
                RHIDevice* device,
                RHICommandBuffer* cmdBuf,
                FRenderStats& outStats) override;

private:
    struct FPreparedStandalonePipeline
    {
        std::unique_ptr<RHIShader> VertexShader;
        std::unique_ptr<RHIShader> FragmentShader;
        std::vector<std::unique_ptr<RHIBindGroupLayout>> BindGroupLayouts;
        std::unique_ptr<RHIPipelineLayout> PipelineLayout;
        std::unique_ptr<RHIPipeline> Pipeline;
    };

    [[nodiscard]] bool EnsureSkyPipeline(RHIDevice* device);
    [[nodiscard]] bool BuildSkyPipeline(RHIDevice* device);
    void SubmitSkyPass(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf);

    static void SortDrawCommands(std::vector<FMeshDrawCommand>& commands) ;
    void SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                            const FScene* scene,
                            RHIDevice* device,
                            RHICommandBuffer* cmdBuf,
                            FRenderStats& outStats) ;

    FMeshPassProcessor m_BasePassProcessor;
    std::unique_ptr<FLightUniformBindingState> m_LightBindingState;
    std::unique_ptr<FObjectUniformBindingState> m_ObjectBindingState;
    std::unique_ptr<FMaterialTextureBindingState> m_MaterialTextureBindingState;
    std::unique_ptr<FMaterialUniformBindingState> m_MaterialBindingState;
    std::unique_ptr<FEnvironmentTextureBindingState> m_EnvironmentTextureBindingState;
    std::unique_ptr<FSkyUniformBindingState> m_SkyBindingState;
    FPreparedStandalonePipeline m_SkyPipeline;
};

} // namespace TE
