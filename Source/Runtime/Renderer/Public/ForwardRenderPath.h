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
class RHIDevice;
class RHIPipeline;
struct FLightUniformBindingState;
struct FObjectUniformBindingState;
struct FMaterialTextureBindingState;
struct FMaterialUniformBindingState;

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
};

} // namespace TE
