// ToyEngine Renderer Module
// FStaticMeshSceneProxy 实现
// 从 TStaticMesh 资产构建 GPU 资源（多 Section 支持）

#include "FStaticMeshSceneProxy.h"
#include "TStaticMesh.h"
#include "RHIDevice.h"
#include "RHIBuffer.h"
#include "RHIShader.h"
#include "RHIPipeline.h"
#include "RHITypes.h"
#include "Log/Log.h"

// 模型 Shader 路径前缀宏（由 Engine 模块传递）
#ifndef TE_PROJECT_ROOT_DIR
    #define TE_PROJECT_ROOT_DIR ""
#endif

namespace TE {

FStaticMeshSceneProxy::FStaticMeshSceneProxy(const TStaticMesh* staticMesh, RHIDevice* device)
{
    if (!device)
    {
        TE_LOG_ERROR("[Renderer] FStaticMeshSceneProxy created with null RHIDevice");
        return;
    }

    if (!staticMesh || !staticMesh->IsValid())
    {
        TE_LOG_ERROR("[Renderer] FStaticMeshSceneProxy created with invalid TStaticMesh");
        return;
    }

    // 1. 创建共享的 Shader 和 Pipeline
    if (!CreateShaderAndPipeline(device))
    {
        TE_LOG_ERROR("[Renderer] FStaticMeshSceneProxy failed to create shader/pipeline");
        return;
    }

    // 2. 为每个 Section 创建独立的 VBO/IBO
    const auto& sections = staticMesh->GetSections();
    m_SectionGPUData.reserve(sections.size());

    for (size_t i = 0; i < sections.size(); ++i)
    {
        const auto& section = sections[i];
        if (section.Vertices.empty() || section.Indices.empty())
            continue;

        FSectionGPUData gpuData;

        // 创建顶点缓冲区
        {
            RHIBufferDesc vbDesc;
            vbDesc.usage = RHIBufferUsage::Vertex;
            vbDesc.size = section.Vertices.size() * sizeof(FStaticMeshVertex);
            vbDesc.initialData = section.Vertices.data();
            vbDesc.debugName = "StaticMesh_Section" + std::to_string(i) + "_VBO";
            gpuData.VertexBuffer = device->CreateBuffer(vbDesc);
        }

        // 创建索引缓冲区
        {
            RHIBufferDesc ibDesc;
            ibDesc.usage = RHIBufferUsage::Index;
            ibDesc.size = section.Indices.size() * sizeof(uint32_t);
            ibDesc.initialData = section.Indices.data();
            ibDesc.debugName = "StaticMesh_Section" + std::to_string(i) + "_IBO";
            gpuData.IndexBuffer = device->CreateBuffer(ibDesc);
            gpuData.IndexCount = static_cast<uint32_t>(section.Indices.size());
        }

        m_SectionGPUData.push_back(std::move(gpuData));
    }

    if (IsValid())
    {
        TE_LOG_INFO("[Renderer] FStaticMeshSceneProxy created: {} sections, {} total vertices, {} total indices",
                    m_SectionGPUData.size(),
                    staticMesh->GetTotalVertexCount(),
                    staticMesh->GetTotalIndexCount());
    }
    else
    {
        TE_LOG_ERROR("[Renderer] FStaticMeshSceneProxy creation failed");
    }
}

bool FStaticMeshSceneProxy::CreateShaderAndPipeline(RHIDevice* device)
{
    // 使用 model shader（支持 Position + Normal + TexCoord + Color）
    std::string shaderDir = std::string(TE_PROJECT_ROOT_DIR) + "Content/Shaders/OpenGL/";

    // 创建顶点着色器
    {
        RHIShaderDesc vsDesc;
        vsDesc.stage = RHIShaderStage::Vertex;
        vsDesc.filePath = shaderDir + "model.vert";
        vsDesc.debugName = "Model_VS";
        m_VertexShader = device->CreateShader(vsDesc);
    }

    // 创建片段着色器
    {
        RHIShaderDesc fsDesc;
        fsDesc.stage = RHIShaderStage::Fragment;
        fsDesc.filePath = shaderDir + "model.frag";
        fsDesc.debugName = "Model_FS";
        m_FragmentShader = device->CreateShader(fsDesc);
    }

    if (!m_VertexShader || !m_FragmentShader)
        return false;

    // 创建管线
    {
        RHIPipelineDesc pipelineDesc;
        pipelineDesc.vertexShader = m_VertexShader.get();
        pipelineDesc.fragmentShader = m_FragmentShader.get();
        pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;

        // FStaticMeshVertex 的顶点布局
        // Position(vec3) + Normal(vec3) + TexCoord(vec2) + Color(vec3) = 44 bytes
        RHIVertexBindingDesc binding;
        binding.binding = 0;
        binding.stride = sizeof(FStaticMeshVertex);
        pipelineDesc.vertexInput.bindings.push_back(binding);

        // location = 0: Position (vec3, offset 0)
        pipelineDesc.vertexInput.attributes.push_back({0, RHIFormat::Float3, offsetof(FStaticMeshVertex, Position)});
        // location = 1: Normal (vec3, offset 12)
        pipelineDesc.vertexInput.attributes.push_back({1, RHIFormat::Float3, offsetof(FStaticMeshVertex, Normal)});
        // location = 2: TexCoord (vec2, offset 24)
        pipelineDesc.vertexInput.attributes.push_back({2, RHIFormat::Float2, offsetof(FStaticMeshVertex, TexCoord)});
        // location = 3: Color (vec3, offset 32)
        pipelineDesc.vertexInput.attributes.push_back({3, RHIFormat::Float3, offsetof(FStaticMeshVertex, Color)});

        // 深度测试
        pipelineDesc.depthStencil.depthTestEnable = true;
        pipelineDesc.depthStencil.depthWriteEnable = true;
        pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Less;

        // 面剔除
        pipelineDesc.rasterization.cullMode = RHICullMode::Back;
        pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;

        pipelineDesc.debugName = "StaticMesh_Pipeline";
        m_Pipeline = device->CreatePipeline(pipelineDesc);
    }

    return m_Pipeline && m_Pipeline->IsValid();
}

void FStaticMeshSceneProxy::GetMeshDrawCommands(std::vector<FMeshDrawCommand>& outCommands) const
{
    if (!IsValid())
        return;

    for (const auto& gpuData : m_SectionGPUData)
    {
        if (!gpuData.VertexBuffer || !gpuData.IndexBuffer)
            continue;

        FMeshDrawCommand cmd;
        cmd.Pipeline = m_Pipeline.get();
        cmd.VertexBuffer = gpuData.VertexBuffer.get();
        cmd.IndexBuffer = gpuData.IndexBuffer.get();
        cmd.IndexCount = gpuData.IndexCount;
        cmd.WorldMatrix = m_WorldMatrix;
        outCommands.push_back(cmd);
    }
}

bool FStaticMeshSceneProxy::IsValid() const
{
    if (!m_VertexShader || !m_FragmentShader || !m_Pipeline || !m_Pipeline->IsValid())
        return false;

    return !m_SectionGPUData.empty();
}

} // namespace TE
