// ToyEngine Renderer Module
// FDeferredRenderPath 实现

#include "DeferredRenderPath.h"

#include "RendererLightUniforms.h"
#include "RendererPassUniforms.h"
#include "RendererBindingSlots.h"
#include "RendererShaderNames.h"
#include "RendererTextureBindings.h"
#include "RendererScene.h"
#include "RenderStats.h"
#include "StaticMesh.h"
#include "ViewInfo.h"
#include "RHIDevice.h"
#include "RHIPipeline.h"
#include "RHIRenderTarget.h"
#include "RHISampler.h"
#include "RHIShader.h"
#include "RHITexture.h"
#include "RHICommandBuffer.h"
#include "RHITypes.h"

#include <algorithm>
#include <cstddef>
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
    desc.entries.push_back({
        binding,
        RHIBindingType::UniformBuffer,
        visibility
    });
    return device ? device->CreateBindGroupLayout(desc) : nullptr;
}

std::unique_ptr<RHIBindGroupLayout> CreateSingleTextureLayout(RHIDevice* device,
                                                              uint32_t binding,
                                                              const char* debugName)
{
    RHIBindGroupLayoutDesc desc;
    desc.debugName = debugName;
    desc.entries.push_back({
        binding,
        RHIBindingType::Texture2D,
        RHIShaderStage::Fragment
    });
    return device ? device->CreateBindGroupLayout(desc) : nullptr;
}

std::unique_ptr<RHIBindGroupLayout> CreateGBufferTexturesLayout(RHIDevice* device)
{
    if (!device)
    {
        return nullptr;
    }

    RHIBindGroupLayoutDesc desc;
    desc.debugName = "DeferredLighting_GBufferTextures_Layout";
    desc.entries.push_back({
        RendererBindings::GBufferAlbedo,
        RHIBindingType::Texture2D,
        RHIShaderStage::Fragment
    });
    desc.entries.push_back({
        RendererBindings::GBufferNormal,
        RHIBindingType::Texture2D,
        RHIShaderStage::Fragment
    });
    desc.entries.push_back({
        RendererBindings::GBufferWorldPosition,
        RHIBindingType::Texture2D,
        RHIShaderStage::Fragment
    });
    desc.entries.push_back({
        RendererBindings::GBufferDepth,
        RHIBindingType::Texture2D,
        RHIShaderStage::Fragment
    });
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
    pipeline.BindGroupLayouts.reserve(layouts.size());
    for (auto& [groupIndex, layout] : layouts)
    {
        (void)groupIndex;
        pipeline.BindGroupLayouts.push_back(std::move(layout));
    }
    pipeline.PipelineLayout = std::move(pipelineLayout);
    return true;
}

void FillStaticMeshVertexInput(RHIVertexInputDesc& vertexInput)
{
    RHIVertexBindingDesc binding;
    binding.binding = 0;
    binding.stride = sizeof(FStaticMeshVertex);
    vertexInput.bindings.push_back(binding);

    vertexInput.attributes.push_back({0, RHIFormat::Float3, offsetof(FStaticMeshVertex, Position)});
    vertexInput.attributes.push_back({1, RHIFormat::Float3, offsetof(FStaticMeshVertex, Normal)});
    vertexInput.attributes.push_back({2, RHIFormat::Float2, offsetof(FStaticMeshVertex, TexCoord)});
    vertexInput.attributes.push_back({3, RHIFormat::Float3, offsetof(FStaticMeshVertex, Color)});
}

} // namespace

FDeferredRenderPath::FDeferredRenderPath()
    : m_GBufferPassProcessor(EMeshPassType::BasePass)
    , m_LightBindingState(std::make_unique<FLightUniformBindingState>())
    , m_ObjectBindingState(std::make_unique<FObjectUniformBindingState>())
    , m_DeferredPassBindingState(std::make_unique<FDeferredPassUniformBindingState>())
    , m_BaseColorTextureBindingState(std::make_unique<FBaseColorTextureBindingState>())
    , m_GBufferTextureBindingState(std::make_unique<FGBufferTextureBindingState>())
{
}

FDeferredRenderPath::~FDeferredRenderPath() = default;

void FDeferredRenderPath::Render(const FScene* scene,
                                 RHIDevice* device,
                                 RHICommandBuffer* cmdBuf,
                                 FRenderStats& outStats)
{
    outStats = {};
    if (!scene || !device || !cmdBuf)
    {
        return;
    }

    const auto& viewInfo = scene->GetViewInfo();
    const auto width = static_cast<uint32_t>(viewInfo.ViewportWidth);
    const auto height = static_cast<uint32_t>(viewInfo.ViewportHeight);
    if (width == 0 || height == 0)
    {
        return;
    }

    if (!EnsureResources(device, width, height))
    {
        return;
    }

    std::vector<FMeshDrawCommand> drawCommands;
    m_GBufferPassProcessor.BuildDrawCommands(scene, drawCommands);
    SortDrawCommands(drawCommands);

    cmdBuf->Begin();

    RHIRenderPassBeginInfo gBufferPassInfo;
    gBufferPassInfo.clearColor[0] = 0.0f;
    gBufferPassInfo.clearColor[1] = 0.0f;
    gBufferPassInfo.clearColor[2] = 0.0f;
    gBufferPassInfo.clearColor[3] = 1.0f;
    gBufferPassInfo.clearDepth = 1.0f;
    gBufferPassInfo.viewport.x = 0;
    gBufferPassInfo.viewport.y = 0;
    gBufferPassInfo.viewport.width = viewInfo.ViewportWidth;
    gBufferPassInfo.viewport.height = viewInfo.ViewportHeight;
    gBufferPassInfo.renderTarget = m_GBuffer.get();

    cmdBuf->BeginRenderPass(gBufferPassInfo);
    SubmitGBufferPass(drawCommands, scene, device, cmdBuf, outStats);
    cmdBuf->EndRenderPass();

    RHIRenderPassBeginInfo lightingPassInfo;
    lightingPassInfo.clearColor[0] = 0.1f;
    lightingPassInfo.clearColor[1] = 0.1f;
    lightingPassInfo.clearColor[2] = 0.1f;
    lightingPassInfo.clearColor[3] = 1.0f;
    lightingPassInfo.clearDepth = 1.0f;
    lightingPassInfo.viewport.x = 0;
    lightingPassInfo.viewport.y = 0;
    lightingPassInfo.viewport.width = viewInfo.ViewportWidth;
    lightingPassInfo.viewport.height = viewInfo.ViewportHeight;

    cmdBuf->BeginRenderPass(lightingPassInfo);
    SubmitLightingPass(scene, device, cmdBuf, outStats);
    cmdBuf->EndRenderPass();

    cmdBuf->End();
}

bool FDeferredRenderPath::EnsureResources(RHIDevice* device, uint32_t width, uint32_t height)
{
    return EnsurePipelines(device) && EnsureGBuffer(device, width, height);
}

bool FDeferredRenderPath::EnsurePipelines(RHIDevice* device)
{
    if (m_GBufferPipeline.Pipeline && m_GBufferPipeline.Pipeline->IsValid() &&
        m_LightingPipeline.Pipeline && m_LightingPipeline.Pipeline->IsValid())
    {
        return true;
    }

    return BuildGBufferPipeline(device) && BuildLightingPipeline(device);
}

bool FDeferredRenderPath::EnsureGBuffer(RHIDevice* device, uint32_t width, uint32_t height)
{
    if (!device)
    {
        return false;
    }

    if (m_GBuffer && m_GBuffer->IsValid() && m_GBufferWidth == width && m_GBufferHeight == height)
    {
        return true;
    }

    RHIRenderTargetDesc desc;
    desc.width = width;
    desc.height = height;
    desc.colorAttachments.push_back({RHIFormat::RGBA8_UNorm, false});
    desc.colorAttachments.push_back({RHIFormat::RGBA8_UNorm, false});
    desc.colorAttachments.push_back({RHIFormat::RGBA32_Float, false});
    desc.hasDepthStencil = true;
    desc.depthStencilAttachment.format = RHIFormat::D32_Float;
    desc.depthStencilAttachment.isDepthStencil = true;
    desc.debugName = "Deferred_GBuffer";

    m_GBuffer = device->CreateRenderTarget(desc);
    if (!m_GBuffer || !m_GBuffer->IsValid())
    {
        m_GBuffer.reset();
        m_GBufferWidth = 0;
        m_GBufferHeight = 0;
        return false;
    }

    m_GBufferWidth = width;
    m_GBufferHeight = height;
    return true;
}

bool FDeferredRenderPath::BuildGBufferPipeline(RHIDevice* device)
{
    if (!device)
    {
        return false;
    }

    RHIShaderDesc vsDesc;
    vsDesc.stage = RHIShaderStage::Vertex;
    vsDesc.logicalName = RendererShaderNames::StaticMeshGBufferVS;
    vsDesc.debugName = "GBuffer_VS";
    m_GBufferPipeline.VertexShader = device->CreateShader(vsDesc);

    RHIShaderDesc fsDesc;
    fsDesc.stage = RHIShaderStage::Fragment;
    fsDesc.logicalName = RendererShaderNames::StaticMeshGBufferPS;
    fsDesc.debugName = "GBuffer_FS";
    m_GBufferPipeline.FragmentShader = device->CreateShader(fsDesc);

    if (!m_GBufferPipeline.VertexShader || !m_GBufferPipeline.FragmentShader)
    {
        return false;
    }

    std::vector<std::pair<uint32_t, std::unique_ptr<RHIBindGroupLayout>>> layouts;
    layouts.push_back({
        RendererBindGroups::PassBlock,
        CreateSingleUniformLayout(device,
                                  RendererBindings::PassBlock,
                                  RHIShaderStage::Vertex,
                                  "DeferredGBuffer_ObjectBlock_Layout")
    });
    layouts.push_back({
        RendererBindGroups::MaterialTextures,
        CreateSingleTextureLayout(device,
                                  RendererBindings::BaseColorTexture,
                                  "DeferredGBuffer_BaseColor_Layout")
    });
    if (!BuildPipelineLayout(device, m_GBufferPipeline, std::move(layouts), "DeferredGBuffer_PipelineLayout"))
    {
        return false;
    }

    RHIPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = m_GBufferPipeline.VertexShader.get();
    pipelineDesc.fragmentShader = m_GBufferPipeline.FragmentShader.get();
    pipelineDesc.layout = m_GBufferPipeline.PipelineLayout.get();
    pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;
    FillStaticMeshVertexInput(pipelineDesc.vertexInput);
    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Less;
    pipelineDesc.rasterization.cullMode = RHICullMode::Back;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.debugName = "Deferred_GBuffer_Pipeline";

    m_GBufferPipeline.Pipeline = device->CreatePipeline(pipelineDesc);
    return m_GBufferPipeline.Pipeline && m_GBufferPipeline.Pipeline->IsValid();
}

bool FDeferredRenderPath::BuildLightingPipeline(RHIDevice* device)
{
    if (!device)
    {
        return false;
    }

    RHIShaderDesc vsDesc;
    vsDesc.stage = RHIShaderStage::Vertex;
    vsDesc.logicalName = RendererShaderNames::DeferredLightingVS;
    vsDesc.debugName = "DeferredLighting_VS";
    m_LightingPipeline.VertexShader = device->CreateShader(vsDesc);

    RHIShaderDesc fsDesc;
    fsDesc.stage = RHIShaderStage::Fragment;
    fsDesc.logicalName = RendererShaderNames::DeferredLightingPS;
    fsDesc.debugName = "DeferredLighting_FS";
    m_LightingPipeline.FragmentShader = device->CreateShader(fsDesc);

    if (!m_LightingPipeline.VertexShader || !m_LightingPipeline.FragmentShader)
    {
        return false;
    }

    std::vector<std::pair<uint32_t, std::unique_ptr<RHIBindGroupLayout>>> layouts;
    layouts.push_back({
        RendererBindGroups::LightBlock,
        CreateSingleUniformLayout(device,
                                  RendererBindings::LightBlock,
                                  RHIShaderStage::Fragment,
                                  "DeferredLighting_LightBlock_Layout")
    });
    layouts.push_back({
        RendererBindGroups::PassBlock,
        CreateSingleUniformLayout(device,
                                  RendererBindings::PassBlock,
                                  RHIShaderStage::Fragment,
                                  "DeferredLighting_PassBlock_Layout")
    });
    layouts.push_back({
        RendererBindGroups::GBufferTextures,
        CreateGBufferTexturesLayout(device)
    });
    if (!BuildPipelineLayout(device, m_LightingPipeline, std::move(layouts), "DeferredLighting_PipelineLayout"))
    {
        return false;
    }

    RHIPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = m_LightingPipeline.VertexShader.get();
    pipelineDesc.fragmentShader = m_LightingPipeline.FragmentShader.get();
    pipelineDesc.layout = m_LightingPipeline.PipelineLayout.get();
    pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;
    pipelineDesc.depthStencil.depthTestEnable = false;
    pipelineDesc.depthStencil.depthWriteEnable = false;
    pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Always;
    pipelineDesc.rasterization.cullMode = RHICullMode::None;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.debugName = "Deferred_Lighting_Pipeline";

    m_LightingPipeline.Pipeline = device->CreatePipeline(pipelineDesc);
    return m_LightingPipeline.Pipeline && m_LightingPipeline.Pipeline->IsValid();
}

void FDeferredRenderPath::SortDrawCommands(std::vector<FMeshDrawCommand>& commands) const
{
    std::sort(commands.begin(), commands.end(),
        [](const FMeshDrawCommand& a, const FMeshDrawCommand& b)
        {
            if (a.VertexBuffer != b.VertexBuffer)
                return a.VertexBuffer < b.VertexBuffer;

            if (a.IndexBuffer != b.IndexBuffer)
                return a.IndexBuffer < b.IndexBuffer;

            if (a.StaticMeshAsset != b.StaticMeshAsset)
                return a.StaticMeshAsset < b.StaticMeshAsset;

            return a.MaterialIndex < b.MaterialIndex;
        });
}

void FDeferredRenderPath::SubmitGBufferPass(const std::vector<FMeshDrawCommand>& commands,
                                            const FScene* scene,
                                            RHIDevice* device,
                                            RHICommandBuffer* cmdBuf,
                                            FRenderStats& outStats) const
{
    if (!m_GBufferPipeline.Pipeline || !m_GBufferPipeline.Pipeline->IsValid())
    {
        return;
    }

    const auto& viewInfo = scene->GetViewInfo();
    const Matrix4 adjustedProjection = device->AdjustProjectionMatrix(viewInfo.ProjectionMatrix);
    const Matrix4 adjustedVP = adjustedProjection * viewInfo.ViewMatrix;

    cmdBuf->BindPipeline(m_GBufferPipeline.Pipeline.get());
    ++outStats.PipelineBindCount;

    RHIBuffer* lastVBO = nullptr;
    RHIBuffer* lastIBO = nullptr;

    for (const auto& cmd : commands)
    {
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

        auto* baseColorTexture = scene->ResolvePreparedBaseColorTexture(cmd.StaticMeshAsset, cmd.MaterialIndex);
        auto* defaultSampler = scene->ResolveDefaultSampler();
        if (baseColorTexture)
        {
            UpdateAndBindBaseColorTexture(device,
                                          cmdBuf,
                                          *m_BaseColorTextureBindingState,
                                          baseColorTexture,
                                          defaultSampler);
        }

        Matrix4 mvp = adjustedVP * cmd.WorldMatrix;
        Matrix3 normalMatrix = cmd.WorldMatrix.GetNormalMatrix();
        UpdateAndBindObjectUniforms(device, cmdBuf, *m_ObjectBindingState, mvp, cmd.WorldMatrix, normalMatrix);

        cmdBuf->DrawIndexed(cmd.IndexCount, cmd.FirstIndex);
        ++outStats.DrawCallCount;
    }
}

void FDeferredRenderPath::SubmitLightingPass(const FScene* scene,
                                             RHIDevice* device,
                                             RHICommandBuffer* cmdBuf,
                                             FRenderStats& outStats) const
{
    if (!m_GBuffer || !m_LightingPipeline.Pipeline || !m_LightingPipeline.Pipeline->IsValid())
    {
        return;
    }

    cmdBuf->BindPipeline(m_LightingPipeline.Pipeline.get());
    ++outStats.PipelineBindCount;

    auto* gbufferSampler = scene->ResolveGBufferSampler();
    UpdateAndBindGBufferTextures(device,
                                 cmdBuf,
                                 *m_GBufferTextureBindingState,
                                 m_GBuffer.get(),
                                 gbufferSampler);

    UpdateAndBindDeferredPassUniforms(device,
                                      cmdBuf,
                                      *m_DeferredPassBindingState,
                                      device->GetBackendTraits().bRTSampleRequiresFlipY,
                                      m_DebugViewMode);
    UpdateAndBindSceneLightUniforms(scene, device, cmdBuf, *m_LightBindingState);

    cmdBuf->Draw(3);
    ++outStats.DrawCallCount;
}

} // namespace TE
