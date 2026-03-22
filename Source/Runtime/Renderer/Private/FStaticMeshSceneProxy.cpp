// ToyEngine Renderer Module
// FStaticMeshSceneProxy 实现
// 构造时通过 RHIDevice 创建 GPU 资源（VBO/IBO/Shader/Pipeline）

#include "FStaticMeshSceneProxy.h"
#include "RHIDevice.h"
#include "RHIBuffer.h"
#include "RHIShader.h"
#include "RHIPipeline.h"
#include "RHITypes.h"
#include "Log/Log.h"

namespace TE {

FStaticMeshSceneProxy::FStaticMeshSceneProxy(const FStaticMeshData& meshData, RHIDevice* device)
{
    if (!device)
    {
        TE_LOG_ERROR("[Renderer] FStaticMeshSceneProxy created with null RHIDevice");
        return;
    }

    // 1. 创建顶点缓冲区
    {
        RHIBufferDesc vbDesc;
        vbDesc.usage = RHIBufferUsage::Vertex;
        vbDesc.size = meshData.Vertices.size() * sizeof(float);
        vbDesc.initialData = meshData.Vertices.data();
        vbDesc.debugName = "StaticMesh_VBO";
        m_VertexBuffer = device->CreateBuffer(vbDesc);
    }

    // 2. 创建索引缓冲区
    {
        RHIBufferDesc ibDesc;
        ibDesc.usage = RHIBufferUsage::Index;
        ibDesc.size = meshData.Indices.size() * sizeof(uint32_t);
        ibDesc.initialData = meshData.Indices.data();
        ibDesc.debugName = "StaticMesh_IBO";
        m_IndexBuffer = device->CreateBuffer(ibDesc);
        m_IndexCount = static_cast<uint32_t>(meshData.Indices.size());
    }

    // 3. 创建着色器
    {
        RHIShaderDesc vsDesc;
        vsDesc.stage = RHIShaderStage::Vertex;
        vsDesc.filePath = meshData.VertexShaderPath;
        vsDesc.debugName = "StaticMesh_VS";
        m_VertexShader = device->CreateShader(vsDesc);

        RHIShaderDesc fsDesc;
        fsDesc.stage = RHIShaderStage::Fragment;
        fsDesc.filePath = meshData.FragmentShaderPath;
        fsDesc.debugName = "StaticMesh_FS";
        m_FragmentShader = device->CreateShader(fsDesc);
    }

    // 4. 创建管线
    {
        RHIPipelineDesc pipelineDesc;
        pipelineDesc.vertexShader = m_VertexShader.get();
        pipelineDesc.fragmentShader = m_FragmentShader.get();
        pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;

        // 顶点输入布局
        RHIVertexBindingDesc binding;
        binding.binding = 0;
        binding.stride = meshData.VertexStride;
        pipelineDesc.vertexInput.bindings.push_back(binding);

        for (const auto& attr : meshData.Attributes)
        {
            RHIVertexAttribute rhiAttr;
            rhiAttr.location = attr.location;
            rhiAttr.format = static_cast<RHIFormat>(attr.format);
            rhiAttr.offset = attr.offset;
            pipelineDesc.vertexInput.attributes.push_back(rhiAttr);
        }

        // 深度测试
        pipelineDesc.depthStencil.depthTestEnable = meshData.DepthTestEnabled;
        pipelineDesc.depthStencil.depthWriteEnable = meshData.DepthWriteEnabled;
        pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Less;

        // 面剔除
        if (meshData.BackfaceCulling)
        {
            pipelineDesc.rasterization.cullMode = RHICullMode::Back;
            pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
        }
        else
        {
            pipelineDesc.rasterization.cullMode = RHICullMode::None;
        }

        pipelineDesc.debugName = "StaticMesh_Pipeline";
        m_Pipeline = device->CreatePipeline(pipelineDesc);
    }

    if (IsValid())
    {
        TE_LOG_INFO("[Renderer] FStaticMeshSceneProxy created: {} vertices, {} indices",
                    meshData.Vertices.size() / (meshData.VertexStride / sizeof(float)),
                    m_IndexCount);
    }
    else
    {
        TE_LOG_ERROR("[Renderer] FStaticMeshSceneProxy creation failed");
    }
}

bool FStaticMeshSceneProxy::GetMeshDrawCommand(FMeshDrawCommand& outCmd) const
{
    if (!IsValid())
        return false;

    outCmd.Pipeline = m_Pipeline.get();
    outCmd.VertexBuffer = m_VertexBuffer.get();
    outCmd.IndexBuffer = m_IndexBuffer.get();
    outCmd.IndexCount = m_IndexCount;
    outCmd.WorldMatrix = m_WorldMatrix;
    return true;
}

bool FStaticMeshSceneProxy::IsValid() const
{
    return m_VertexBuffer && m_IndexBuffer && m_VertexShader && m_FragmentShader
           && m_Pipeline && m_Pipeline->IsValid();
}

} // namespace TE
