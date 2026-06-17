// ToyEngine Renderer Module
// FForwardRenderPath 实现

#include "ForwardRenderPath.h"

#include "RendererLightUniforms.h"
#include "RendererPassUniforms.h"
#include "RendererBindingSlots.h"
#include "RendererShaderNames.h"
#include "RendererTextureBindings.h"
#include "RendererScene.h"
#include "RenderStats.h"
#include "ViewInfo.h"
#include "RHIBindGroup.h"
#include "RHICommandBuffer.h"
#include "RHIDevice.h"
#include "RHIPipeline.h"
#include "RHIShader.h"
#include "RHITypes.h"

#include <algorithm>
#include <utility>

namespace TE {

namespace {

std::unique_ptr<RHIBindGroupLayout> CreateSingleUniformLayout(RHIDevice* device,
                                                              uint32_t binding,
                                                              RHIShaderStage visibility,
                                                              const char* debugName)
{
    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({binding, RHIBindingType::UniformBuffer, visibility});
    return device ? device->CreateBindGroupLayout(desc) : nullptr;
}

std::unique_ptr<RHIBindGroupLayout> CreateEnvironmentTexturesLayout(RHIDevice* device, const char* debugName)
{
    if (!device)
    {
        return nullptr;
    }

    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({RendererBindings::IrradianceMap, RHIBindingType::TextureCube, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::PrefilterMap, RHIBindingType::TextureCube, RHIShaderStage::Fragment});
    desc.entries.push_back({RendererBindings::BRDFLUT, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    return device->CreateBindGroupLayout(desc);
}

template <typename TPipelineCache>
bool BuildPipelineLayout(RHIDevice* device,
                         TPipelineCache& pipeline,
                         std::vector<std::pair<uint32_t, std::unique_ptr<RHIBindGroupLayout>>> layouts,
                         const char* debugName)
{
    if (!device)
    {
        return false;
    }

    RHIPipelineLayoutDesc desc;
    desc.debugName = debugName;
    desc.bindGroupLayouts.reserve(layouts.size());
    for (const auto& [groupIndex, layout] : layouts)
    {
        if (!layout || !layout->IsValid())
        {
            return false;
        }
        desc.bindGroupLayouts.push_back({groupIndex, layout.get()});
    }

    auto pipelineLayout = device->CreatePipelineLayout(desc);
    if (!pipelineLayout || !pipelineLayout->IsValid())
    {
        return false;
    }

    pipeline.BindGroupLayouts.clear();
    for (auto& [groupIndex, layout] : layouts)
    {
        (void)groupIndex;
        pipeline.BindGroupLayouts.push_back(std::move(layout));
    }
    pipeline.PipelineLayout = std::move(pipelineLayout);
    return true;
}

} // namespace

FForwardRenderPath::FForwardRenderPath()
    : m_BasePassProcessor(EMeshPassType::BasePass)
    , m_LightBindingState(std::make_unique<FLightUniformBindingState>())
    , m_ObjectBindingState(std::make_unique<FObjectUniformBindingState>())
    , m_MaterialTextureBindingState(std::make_unique<FMaterialTextureBindingState>())
    , m_MaterialBindingState(std::make_unique<FMaterialUniformBindingState>())
    , m_EnvironmentTextureBindingState(std::make_unique<FEnvironmentTextureBindingState>())
    , m_SkyBindingState(std::make_unique<FSkyUniformBindingState>())
{
}

FForwardRenderPath::~FForwardRenderPath() = default;

void FForwardRenderPath::Render(const FScene* scene,
                                RHIDevice* device,
                                RHICommandBuffer* cmdBuf,
                                FRenderStats& outStats)
{
    outStats = {};
    if (!scene || !device || !cmdBuf)
    {
        return;
    }

    std::vector<FMeshDrawCommand> drawCommands;
    m_BasePassProcessor.BuildDrawCommands(scene, drawCommands);
    if (drawCommands.empty())
    {
        return;
    }

    SortDrawCommands(drawCommands);

    const auto& viewInfo = scene->GetViewInfo();

    cmdBuf->Begin();

    RHIRenderPassBeginInfo passInfo;
    passInfo.clearColor[0] = 0.1f;
    passInfo.clearColor[1] = 0.1f;
    passInfo.clearColor[2] = 0.1f;
    passInfo.clearColor[3] = 1.0f;
    passInfo.clearDepth = 1.0f;
    passInfo.viewport.x = 0;
    passInfo.viewport.y = 0;
    passInfo.viewport.width = viewInfo.ViewportWidth;
    passInfo.viewport.height = viewInfo.ViewportHeight;

    cmdBuf->BeginRenderPass(passInfo);
    SubmitSkyPass(scene, device, cmdBuf);
    SubmitDrawCommands(drawCommands, scene, device, cmdBuf, outStats);
    cmdBuf->EndRenderPass();

    cmdBuf->End();
}

bool FForwardRenderPath::EnsureSkyPipeline(RHIDevice* device)
{
    if (m_SkyPipeline.Pipeline && m_SkyPipeline.Pipeline->IsValid())
    {
        return true;
    }
    return BuildSkyPipeline(device);
}

bool FForwardRenderPath::BuildSkyPipeline(RHIDevice* device)
{
    if (!device)
    {
        return false;
    }

    RHIShaderDesc vsDesc;
    vsDesc.stage = RHIShaderStage::Vertex;
    vsDesc.logicalName = RendererShaderNames::SkyVS;
    vsDesc.debugName = "Sky_VS";
    m_SkyPipeline.VertexShader = device->CreateShader(vsDesc);

    RHIShaderDesc fsDesc;
    fsDesc.stage = RHIShaderStage::Fragment;
    fsDesc.logicalName = RendererShaderNames::SkyPS;
    fsDesc.debugName = "Sky_FS";
    m_SkyPipeline.FragmentShader = device->CreateShader(fsDesc);

    if (!m_SkyPipeline.VertexShader || !m_SkyPipeline.FragmentShader)
    {
        return false;
    }

    std::vector<std::pair<uint32_t, std::unique_ptr<RHIBindGroupLayout>>> layouts;
    layouts.push_back({
        RendererBindGroups::PassBlock,
        CreateSingleUniformLayout(device,
                                  RendererBindings::PassBlock,
                                  RHIShaderStage::Fragment,
                                  "Sky_PassBlock_Layout")
    });
    layouts.push_back({
        RendererBindGroups::Environment,
        CreateEnvironmentTexturesLayout(device, "Sky_EnvironmentTextures_Layout")
    });

    if (!BuildPipelineLayout(device, m_SkyPipeline, std::move(layouts), "Sky_PipelineLayout"))
    {
        return false;
    }

    RHIPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = m_SkyPipeline.VertexShader.get();
    pipelineDesc.fragmentShader = m_SkyPipeline.FragmentShader.get();
    pipelineDesc.layout = m_SkyPipeline.PipelineLayout.get();
    pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;
    pipelineDesc.depthStencil.depthTestEnable = false;
    pipelineDesc.depthStencil.depthWriteEnable = false;
    pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Always;
    pipelineDesc.rasterization.cullMode = RHICullMode::None;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.debugName = "Sky_Pipeline";

    m_SkyPipeline.Pipeline = device->CreatePipeline(pipelineDesc);
    return m_SkyPipeline.Pipeline && m_SkyPipeline.Pipeline->IsValid();
}

void FForwardRenderPath::SubmitSkyPass(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf)
{
    if (!scene || !device || !cmdBuf || !EnsureSkyPipeline(device))
    {
        return;
    }

    const auto* environmentResources = scene->ResolveEnvironmentIBLResources();
    auto* environmentSampler = scene->ResolveEnvironmentSampler();
    if (!environmentResources || !environmentSampler)
    {
        return;
    }

    const auto& viewInfo = scene->GetViewInfo();
    const Matrix4 adjustedProjection = device->AdjustProjectionMatrix(viewInfo.ProjectionMatrix);
    const Matrix4 invViewProjection = (adjustedProjection * viewInfo.ViewMatrix).Inverse();

    cmdBuf->BindPipeline(m_SkyPipeline.Pipeline.get());
    UpdateAndBindSkyUniforms(device, cmdBuf, *m_SkyBindingState, invViewProjection, viewInfo.CameraPosition);
    UpdateAndBindEnvironmentTextures(device,
                                     cmdBuf,
                                     *m_EnvironmentTextureBindingState,
                                     environmentResources,
                                     environmentSampler);
    cmdBuf->Draw(3);
}

void FForwardRenderPath::SortDrawCommands(std::vector<FMeshDrawCommand>& commands)
{
    std::ranges::sort(commands,
                      [](const FMeshDrawCommand& a, const FMeshDrawCommand& b)
                      {
                          if (a.PipelineKey.Pass != b.PipelineKey.Pass)
                              return static_cast<uint8_t>(a.PipelineKey.Pass) < static_cast<uint8_t>(b.PipelineKey.Pass);

                          if (a.PipelineKey.MaterialDomain != b.PipelineKey.MaterialDomain)
                              return static_cast<uint8_t>(a.PipelineKey.MaterialDomain) < static_cast<uint8_t>(b.PipelineKey.MaterialDomain);

                          if (a.PipelineKey.VertexFactory != b.PipelineKey.VertexFactory)
                              return static_cast<uint8_t>(a.PipelineKey.VertexFactory) < static_cast<uint8_t>(b.PipelineKey.VertexFactory);

                          if (a.VertexBuffer != b.VertexBuffer)
                              return a.VertexBuffer < b.VertexBuffer;

                          return a.IndexBuffer < b.IndexBuffer;
                      });
}

void FForwardRenderPath::SubmitDrawCommands(const std::vector<FMeshDrawCommand>& commands,
                                            const FScene* scene,
                                            RHIDevice* device,
                                            RHICommandBuffer* cmdBuf,
                                            FRenderStats& outStats)
{
    const auto& viewInfo = scene->GetViewInfo();

    const Matrix4 adjustedProjection = device->AdjustProjectionMatrix(viewInfo.ProjectionMatrix);
    const Matrix4 adjustedVP = adjustedProjection * viewInfo.ViewMatrix;

    RHIPipeline* lastPipeline = nullptr;
    RHIBuffer* lastVBO = nullptr;
    RHIBuffer* lastIBO = nullptr;

    const auto* environmentResources = scene->ResolveEnvironmentIBLResources();
    auto* environmentSampler = scene->ResolveEnvironmentSampler();

    for (const auto& cmd : commands)
    {
        auto* pipeline = scene->ResolvePreparedPipeline(cmd.PipelineKey);
        if (!pipeline || !pipeline->IsValid())
        {
            continue;
        }

        if (pipeline != lastPipeline)
        {
            cmdBuf->BindPipeline(pipeline);
            lastPipeline = pipeline;
            ++outStats.PipelineBindCount;

            lastVBO = nullptr;
            lastIBO = nullptr;
        }

        if (cmd.VertexBuffer != lastVBO)
        {
            cmdBuf->BindVertexBuffer(cmd.VertexBuffer);
            lastVBO = cmd.VertexBuffer;
            ++outStats.VBOBindCount;
        }

        if (cmd.IndexBuffer != lastIBO)
        {
            cmdBuf->BindIndexBuffer(cmd.IndexBuffer);
            lastIBO = cmd.IndexBuffer;
            ++outStats.IBOBindCount;
        }

        const auto* material = scene->ResolveMaterial(cmd.StaticMeshAsset, cmd.MaterialIndex);
        const auto* materialTextures = scene->ResolvePreparedMaterialTextures(cmd.StaticMeshAsset, cmd.MaterialIndex);
        auto* defaultSampler = scene->ResolveDefaultSampler();
        if (materialTextures)
        {
            UpdateAndBindMaterialTextures(device,
                                          cmdBuf,
                                          *m_MaterialTextureBindingState,
                                          materialTextures,
                                          defaultSampler);
        }
        UpdateAndBindMaterialUniforms(device, cmdBuf, *m_MaterialBindingState, material, viewInfo.CameraPosition);
        UpdateAndBindEnvironmentTextures(device,
                                         cmdBuf,
                                         *m_EnvironmentTextureBindingState,
                                         environmentResources,
                                         environmentSampler);

        Matrix4 mvp = adjustedVP * cmd.WorldMatrix;
        Matrix3 normalMatrix = cmd.WorldMatrix.GetNormalMatrix();

        UpdateAndBindObjectUniforms(device, cmdBuf, *m_ObjectBindingState, mvp, cmd.WorldMatrix, normalMatrix);

        UpdateAndBindSceneLightUniforms(scene, device, cmdBuf, *m_LightBindingState);

        cmdBuf->DrawIndexed(cmd.IndexCount, cmd.FirstIndex);
        ++outStats.DrawCallCount;
    }
}

} // namespace TE
