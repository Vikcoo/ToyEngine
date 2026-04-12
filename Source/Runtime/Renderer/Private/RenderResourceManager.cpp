// ToyEngine Renderer Module
// FRenderResourceManager 实现

#include "RenderResourceManager.h"

#include "StaticMeshRenderData.h"
#include "StaticMeshSceneProxy.h"
#include "RHIDevice.h"
#include "RHIPipeline.h"
#include "RHIShader.h"
#include "RHITypes.h"
#include "StaticMesh.h"

#include <cstddef>
#include <string>

// 模型 Shader 路径前缀宏（由 CMake 传递）
#ifndef TE_PROJECT_ROOT_DIR
    #define TE_PROJECT_ROOT_DIR ""
#endif

namespace TE {

FRenderResourceManager::FRenderResourceManager(RHIDevice* device)
    : m_Device(device)
{
}

FRenderResourceManager::~FRenderResourceManager() = default;

bool FRenderResourceManager::PrepareStaticMeshProxy(FStaticMeshSceneProxy& proxy)
{
    const auto& staticMesh = proxy.GetStaticMeshAsset();
    if (!staticMesh || !staticMesh->IsValid())
    {
        return false;
    }

    auto renderData = GetOrCreateStaticMeshRenderData(staticMesh);
    if (!renderData)
    {
        return false;
    }

    // 在 Primitive 注册阶段预热 pipeline，保证后续渲染阶段可直接解析。
    if (!EnsureStaticMeshPipeline())
    {
        return false;
    }

    return proxy.SetRenderResources(std::move(renderData));
}

void FRenderResourceManager::PurgeExpiredStaticMeshRenderData()
{
    for (auto it = m_StaticMeshRenderDataCache.begin(); it != m_StaticMeshRenderDataCache.end();)
    {
        if (it->second.expired())
        {
            it = m_StaticMeshRenderDataCache.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

std::shared_ptr<const FStaticMeshRenderData> FRenderResourceManager::GetOrCreateStaticMeshRenderData(
    const std::shared_ptr<StaticMesh>& staticMesh)
{
    if (!m_Device || !staticMesh)
    {
        return nullptr;
    }

    const StaticMesh* key = staticMesh.get();
    const auto found = m_StaticMeshRenderDataCache.find(key);
    if (found != m_StaticMeshRenderDataCache.end())
    {
        auto cached = found->second.lock();
        if (cached && cached->IsValid())
        {
            return cached;
        }
    }

    auto newRenderData = FStaticMeshRenderData::Create(*staticMesh, *m_Device);
    if (!newRenderData || !newRenderData->IsValid())
    {
        return nullptr;
    }

    m_StaticMeshRenderDataCache[key] = newRenderData;
    return newRenderData;
}

bool FRenderResourceManager::EnsureStaticMeshPipeline()
{
    if (m_StaticMeshPipeline && m_StaticMeshPipeline->IsValid())
    {
        return true;
    }

    if (!m_Device)
    {
        return false;
    }

    const std::string shaderDir = std::string(TE_PROJECT_ROOT_DIR) + "Content/Shaders/OpenGL/";

    RHIShaderDesc vsDesc;
    vsDesc.stage = RHIShaderStage::Vertex;
    vsDesc.filePath = shaderDir + "model.vert";
    vsDesc.debugName = "Model_VS";
    m_StaticMeshVertexShader = m_Device->CreateShader(vsDesc);

    RHIShaderDesc fsDesc;
    fsDesc.stage = RHIShaderStage::Fragment;
    fsDesc.filePath = shaderDir + "model.frag";
    fsDesc.debugName = "Model_FS";
    m_StaticMeshFragmentShader = m_Device->CreateShader(fsDesc);

    if (!m_StaticMeshVertexShader || !m_StaticMeshFragmentShader)
    {
        return false;
    }

    RHIPipelineDesc pipelineDesc;
    pipelineDesc.vertexShader = m_StaticMeshVertexShader.get();
    pipelineDesc.fragmentShader = m_StaticMeshFragmentShader.get();
    pipelineDesc.topology = RHIPrimitiveTopology::TriangleList;

    RHIVertexBindingDesc binding;
    binding.binding = 0;
    binding.stride = sizeof(FStaticMeshVertex);
    pipelineDesc.vertexInput.bindings.push_back(binding);

    pipelineDesc.vertexInput.attributes.push_back({0, RHIFormat::Float3, offsetof(FStaticMeshVertex, Position)});
    pipelineDesc.vertexInput.attributes.push_back({1, RHIFormat::Float3, offsetof(FStaticMeshVertex, Normal)});
    pipelineDesc.vertexInput.attributes.push_back({2, RHIFormat::Float2, offsetof(FStaticMeshVertex, TexCoord)});
    pipelineDesc.vertexInput.attributes.push_back({3, RHIFormat::Float3, offsetof(FStaticMeshVertex, Color)});

    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Less;
    pipelineDesc.rasterization.cullMode = RHICullMode::Back;
    pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
    pipelineDesc.debugName = "StaticMesh_Pipeline";

    m_StaticMeshPipeline = m_Device->CreatePipeline(pipelineDesc);
    return m_StaticMeshPipeline && m_StaticMeshPipeline->IsValid();
}

RHIPipeline* FRenderResourceManager::GetPreparedPipeline(EMeshPipelineKey pipelineKey) const
{
    switch (pipelineKey)
    {
    case EMeshPipelineKey::StaticMeshLit:
        return m_StaticMeshPipeline.get();
    default:
        return nullptr;
    }
}

} // namespace TE
