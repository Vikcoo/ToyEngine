// ToyEngine Renderer Module
// Vulkan 阶段 B 静态网格能力验证路径实现

#include "StaticMeshValidationRenderPath.h"

#include "RendererDepthConvention.h"
#include "RendererShaderNames.h"
#include "RenderStats.h"
#include "RHI.h"
#include "Log/Log.h"
#include "Math/Matrix.h"
#include "Math/ScalarMath.h"
#include "Math/Vector.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace TE {

namespace {

struct FValidationVertex
{
    float Position[3];
    float TexCoord[2];
    float Color[3];
};

struct alignas(16) FValidationObjectBlock
{
    Matrix4 MVP;
};

constexpr std::array<FValidationVertex, 24> ValidationVertices = {{
    {{-1, -1,  1}, {0, 1}, {1.0f, 0.7f, 0.7f}}, {{ 1, -1,  1}, {1, 1}, {1.0f, 0.7f, 0.7f}},
    {{ 1,  1,  1}, {1, 0}, {1.0f, 0.7f, 0.7f}}, {{-1,  1,  1}, {0, 0}, {1.0f, 0.7f, 0.7f}},
    {{ 1, -1, -1}, {0, 1}, {0.7f, 1.0f, 0.7f}}, {{-1, -1, -1}, {1, 1}, {0.7f, 1.0f, 0.7f}},
    {{-1,  1, -1}, {1, 0}, {0.7f, 1.0f, 0.7f}}, {{ 1,  1, -1}, {0, 0}, {0.7f, 1.0f, 0.7f}},
    {{ 1, -1,  1}, {0, 1}, {0.7f, 0.7f, 1.0f}}, {{ 1, -1, -1}, {1, 1}, {0.7f, 0.7f, 1.0f}},
    {{ 1,  1, -1}, {1, 0}, {0.7f, 0.7f, 1.0f}}, {{ 1,  1,  1}, {0, 0}, {0.7f, 0.7f, 1.0f}},
    {{-1, -1, -1}, {0, 1}, {1.0f, 1.0f, 0.7f}}, {{-1, -1,  1}, {1, 1}, {1.0f, 1.0f, 0.7f}},
    {{-1,  1,  1}, {1, 0}, {1.0f, 1.0f, 0.7f}}, {{-1,  1, -1}, {0, 0}, {1.0f, 1.0f, 0.7f}},
    {{-1,  1,  1}, {0, 1}, {0.7f, 1.0f, 1.0f}}, {{ 1,  1,  1}, {1, 1}, {0.7f, 1.0f, 1.0f}},
    {{ 1,  1, -1}, {1, 0}, {0.7f, 1.0f, 1.0f}}, {{-1,  1, -1}, {0, 0}, {0.7f, 1.0f, 1.0f}},
    {{-1, -1, -1}, {0, 1}, {1.0f, 0.7f, 1.0f}}, {{ 1, -1, -1}, {1, 1}, {1.0f, 0.7f, 1.0f}},
    {{ 1, -1,  1}, {1, 0}, {1.0f, 0.7f, 1.0f}}, {{-1, -1,  1}, {0, 0}, {1.0f, 0.7f, 1.0f}},
}};

constexpr std::array<uint16_t, 36> ValidationIndices = {
     0,  1,  2,  2,  3,  0,  4,  5,  6,  6,  7,  4,
     8,  9, 10, 10, 11,  8, 12, 13, 14, 14, 15, 12,
    16, 17, 18, 18, 19, 16, 20, 21, 22, 22, 23, 20,
};

// 非对称色块可同时暴露上传行距、UV 原点和采样绑定错误。
constexpr std::array<uint8_t, 4 * 4 * 4> ValidationTexture = {
    255,  32,  32, 255, 255,  32,  32, 255,  32, 255,  32, 255, 255, 255, 255, 255,
    255,  32,  32, 255, 255, 128,  32, 255,  32, 255,  32, 255,  32, 255,  32, 255,
     32,  64, 255, 255,  32,  64, 255, 255, 255,  32, 255, 255, 255,  32, 255, 255,
    255, 255, 255, 255,  32,  64, 255, 255, 255,  32, 255, 255,  32,  32,  32, 255,
};

} // namespace

FStaticMeshValidationRenderPath::FStaticMeshValidationRenderPath() = default;

FStaticMeshValidationRenderPath::~FStaticMeshValidationRenderPath()
{
    if (m_Device)
    {
        m_Device->WaitIdle();
    }
}

bool FStaticMeshValidationRenderPath::EnsureResources(RHIDevice* device)
{
    if (m_Pipeline)
    {
        return true;
    }
    m_Device = device;

    RHIBufferDesc vertexDesc;
    vertexDesc.usage = RHIBufferUsage::Vertex;
    vertexDesc.memoryUsage = RHIMemoryUsage::GPUOnly;
    vertexDesc.size = sizeof(ValidationVertices);
    vertexDesc.initialData = ValidationVertices.data();
    vertexDesc.debugName = "StageB_ValidationVertexBuffer";
    m_VertexBuffer = device->CreateBuffer(vertexDesc);

    RHIBufferDesc indexDesc;
    indexDesc.usage = RHIBufferUsage::Index;
    indexDesc.memoryUsage = RHIMemoryUsage::GPUOnly;
    indexDesc.size = sizeof(ValidationIndices);
    indexDesc.initialData = ValidationIndices.data();
    indexDesc.debugName = "StageB_ValidationIndexBuffer";
    m_IndexBuffer = device->CreateBuffer(indexDesc);

    RHITextureDesc textureDesc;
    textureDesc.width = 4;
    textureDesc.height = 4;
    textureDesc.format = RHIFormat::RGBA8_sRGB;
    textureDesc.usage = RHITextureUsage::ShaderResource | RHITextureUsage::CopyDestination;
    textureDesc.mipLevels = 1;
    textureDesc.initialData = ValidationTexture.data();
    textureDesc.initialDataSize = ValidationTexture.size();
    textureDesc.initialDataRowPitch = 4 * 4;
    textureDesc.generateMips = false;
    textureDesc.srgb = true;
    textureDesc.debugName = "StageB_OrientationTexture";
    m_Texture = device->CreateTexture(textureDesc);

    RHISamplerDesc samplerDesc;
    samplerDesc.minFilter = RHITextureFilter::Nearest;
    samplerDesc.magFilter = RHITextureFilter::Nearest;
    samplerDesc.addressU = RHITextureAddressMode::ClampToEdge;
    samplerDesc.addressV = RHITextureAddressMode::ClampToEdge;
    samplerDesc.debugName = "StageB_ValidationSampler";
    m_Sampler = device->CreateSampler(samplerDesc);

    m_VertexShader = device->CreateShader({RHIShaderStage::Vertex,
                                            RendererShaderNames::StageBValidationVS,
                                            "main", "StageB_ValidationVS"});
    m_FragmentShader = device->CreateShader({RHIShaderStage::Fragment,
                                              RendererShaderNames::StageBValidationPS,
                                              "main", "StageB_ValidationPS"});

    RHIBindGroupLayoutDesc objectLayoutDesc;
    objectLayoutDesc.entries.push_back({0, RHIBindingType::DynamicUniformBuffer, RHIShaderStage::Vertex});
    objectLayoutDesc.debugName = "StageB_ObjectLayout";
    m_ObjectLayout = device->CreateBindGroupLayout(objectLayoutDesc);

    RHIBindGroupLayoutDesc textureLayoutDesc;
    textureLayoutDesc.entries.push_back({0, RHIBindingType::Texture2D, RHIShaderStage::Fragment});
    textureLayoutDesc.debugName = "StageB_TextureLayout";
    m_TextureLayout = device->CreateBindGroupLayout(textureLayoutDesc);

    if (!m_VertexBuffer || !m_IndexBuffer || !m_Texture || !m_Sampler ||
        !m_VertexShader || !m_FragmentShader || !m_ObjectLayout || !m_TextureLayout)
    {
        TE_LOG_ERROR("[Renderer] Stage B validation resource creation failed");
        return false;
    }

    RHIPipelineLayoutDesc pipelineLayoutDesc;
    pipelineLayoutDesc.bindGroupLayouts.push_back({0, m_ObjectLayout.get()});
    pipelineLayoutDesc.bindGroupLayouts.push_back({1, m_TextureLayout.get()});
    pipelineLayoutDesc.debugName = "StageB_PipelineLayout";
    m_PipelineLayout = device->CreatePipelineLayout(pipelineLayoutDesc);

    RHIBindGroupDesc textureBindGroupDesc;
    textureBindGroupDesc.layout = m_TextureLayout.get();
    textureBindGroupDesc.entries.push_back({0, RHIBindingType::Texture2D, nullptr, 0, 0,
                                            m_Texture.get(), m_Sampler.get()});
    textureBindGroupDesc.debugName = "StageB_TextureBindGroup";
    m_TextureBindGroup = device->CreateBindGroup(textureBindGroupDesc);

    if (!m_PipelineLayout || !m_TextureBindGroup)
    {
        TE_LOG_ERROR("[Renderer] Stage B descriptor creation failed");
        return false;
    }

    RHIPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = m_VertexShader.get();
    pipelineDesc.fragmentShader = m_FragmentShader.get();
    pipelineDesc.layout = m_PipelineLayout.get();
    pipelineDesc.vertexInput.bindings.push_back({0, sizeof(FValidationVertex)});
    pipelineDesc.vertexInput.attributes.push_back({0, RHIFormat::Float3, offsetof(FValidationVertex, Position)});
    pipelineDesc.vertexInput.attributes.push_back({1, RHIFormat::Float2, offsetof(FValidationVertex, TexCoord)});
    pipelineDesc.vertexInput.attributes.push_back({2, RHIFormat::Float3, offsetof(FValidationVertex, Color)});
    pipelineDesc.rasterization.cullMode = RHICullMode::Back;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = RendererDepth::CompareOp;
    pipelineDesc.rendering.colorAttachmentFormats.push_back(device->GetBackBufferColorFormat());
    pipelineDesc.rendering.depthStencilFormat = device->GetBackBufferDepthFormat();
    pipelineDesc.debugName = "StageB_StaticMeshPipeline";
    m_Pipeline = device->CreatePipeline(pipelineDesc);
    if (!m_Pipeline)
    {
        TE_LOG_ERROR("[Renderer] Stage B graphics pipeline creation failed");
        return false;
    }

    TE_LOG_INFO("[Renderer] Vulkan Stage B validation path ready: indexed cube + texture + Reversed-Z");
    return true;
}

void FStaticMeshValidationRenderPath::Render(RHIDevice* device,
                                             RHICommandBuffer* commandBuffer,
                                             FRenderStats& outStats)
{
    if (!device || !commandBuffer || !EnsureResources(device))
    {
        return;
    }

    RHIRenderPassBeginInfo passInfo;
    passInfo.clearColor[0] = 0.015f;
    passInfo.clearColor[1] = 0.025f;
    passInfo.clearColor[2] = 0.055f;
    passInfo.clearColor[3] = 1.0f;
    passInfo.clearDepth = RendererDepth::ClearValue;
    commandBuffer->BeginRenderPass(passInfo);
    commandBuffer->BindPipeline(m_Pipeline.get());
    commandBuffer->BindVertexBuffer(m_VertexBuffer.get());
    commandBuffer->BindIndexBuffer(m_IndexBuffer.get(), RHIIndexType::UInt16);

    m_Rotation += 0.008f;
    const Matrix4 model = Matrix4::Rotate(m_Rotation, Vector3(0.3f, 1.0f, 0.1f));
    const Matrix4 view = Matrix4::LookAtRH(Vector3(3.4f, 2.4f, 4.5f), Vector3::Zero, Vector3::Up);
    const Matrix4 projection = Matrix4::PerspectiveRH_ZO(Math::DegToRad(55.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    const Matrix4 adjustedProjection = device->AdjustProjectionMatrix(RendererDepth::BuildProjection(projection));
    const FValidationObjectBlock objectBlock{adjustedProjection * view * model};
    if (!AllocateAndBindTransientUniform(device, commandBuffer, m_ObjectBindingState,
                                         &objectBlock, sizeof(objectBlock), 0, 0,
                                         RHIShaderStage::Vertex, "StageB_ObjectDynamicBindGroup"))
    {
        TE_LOG_ERROR("[Renderer] Stage B dynamic uniform binding failed");
        commandBuffer->EndRenderPass();
        return;
    }
    commandBuffer->SetBindGroup(1, m_TextureBindGroup.get());
    commandBuffer->DrawIndexed(static_cast<uint32_t>(ValidationIndices.size()));
    commandBuffer->EndRenderPass();

    outStats.DrawCallCount = 1;
    outStats.PipelineBindCount = 1;
    outStats.VBOBindCount = 1;
    outStats.IBOBindCount = 1;
}

} // namespace TE
