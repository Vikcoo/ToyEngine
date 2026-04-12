// ToyEngine Renderer Module
// FScene 实现 - 渲染场景与资产级渲染资源管理

#include "RendererScene.h"

#include "StaticMeshSceneProxy.h"
#include "Log/Log.h"
#include "RHIDevice.h"
#include "RHIPipeline.h"
#include "RHIShader.h"
#include "RHITypes.h"
#include "TStaticMesh.h"

#include <cstddef>
#include <string>

// 模型 Shader 路径前缀宏（由 CMake 传递）
#ifndef TE_PROJECT_ROOT_DIR
    #define TE_PROJECT_ROOT_DIR ""
#endif

namespace TE {

FScene::FScene(RHIDevice* device)
    : m_Device(device)
{
}

FScene::~FScene() = default;

bool FScene::AddPrimitive(const PrimitiveComponent* primitiveComponent,
                          FPrimitiveComponentId primitiveComponentId,
                          std::unique_ptr<FPrimitiveSceneProxy> proxy)
{
    if (!primitiveComponent || !proxy || !primitiveComponentId.IsValid())
    {
        TE_LOG_WARN("[Renderer] FScene::AddPrimitive called with invalid primitive/proxy/id");
        return false;
    }

    RemovePrimitive(primitiveComponentId);
    return InsertPrimitive(primitiveComponentId, primitiveComponent, std::move(proxy));
}

void FScene::RemovePrimitive(FPrimitiveComponentId primitiveComponentId)
{
    if (!primitiveComponentId.IsValid())
    {
        return;
    }

    const auto it = m_PrimitiveStorage.find(primitiveComponentId);
    if (it == m_PrimitiveStorage.end())
    {
        return;
    }

    m_PrimitiveStorage.erase(it);
    RebuildPrimitiveView();
    TE_LOG_INFO("[Renderer] FScene::RemovePrimitive id={}, total primitives: {}",
                primitiveComponentId.Value, m_PrimitiveStorage.size());
}

void FScene::UpdatePrimitiveTransform(FPrimitiveComponentId primitiveComponentId, const Matrix4& worldMatrix)
{
    if (!primitiveComponentId.IsValid())
    {
        return;
    }

    const auto it = m_PrimitiveStorage.find(primitiveComponentId);
    if (it == m_PrimitiveStorage.end())
    {
        return;
    }

    auto* proxy = it->second->GetProxy();
    if (!proxy)
    {
        return;
    }
    proxy->SetWorldMatrix(worldMatrix);
}

std::shared_ptr<const FStaticMeshRenderData> FScene::GetStaticMeshRenderData(const std::shared_ptr<StaticMesh>& staticMesh)
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

RHIPipeline* FScene::GetStaticMeshPipeline()
{
    if (!EnsureStaticMeshPipeline())
    {
        TE_LOG_ERROR("[Renderer] FScene failed to initialize static mesh pipeline");
        return nullptr;
    }
    return m_StaticMeshPipeline.get();
}

bool FScene::EnsureStaticMeshPipeline()
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

bool FScene::InsertPrimitive(FPrimitiveComponentId primitiveComponentId,
                             const PrimitiveComponent* primitiveComponent,
                             std::unique_ptr<FPrimitiveSceneProxy> proxy)
{
    if (!primitiveComponent || !proxy || !primitiveComponentId.IsValid())
    {
        TE_LOG_WARN("[Renderer] FScene::InsertPrimitive called with invalid primitive/proxy/id");
        return false;
    }

    auto sceneInfo = std::make_unique<FPrimitiveSceneInfo>(primitiveComponentId, primitiveComponent, std::move(proxy));
    m_PrimitiveStorage[primitiveComponentId] = std::move(sceneInfo);
    RebuildPrimitiveView();
    TE_LOG_INFO("[Renderer] FScene::InsertPrimitive id={}, component={}, total primitives: {}",
                primitiveComponentId.Value, static_cast<const void*>(primitiveComponent), m_PrimitiveStorage.size());
    return true;
}

void FScene::RebuildPrimitiveView()
{
    m_Primitives.clear();
    m_Primitives.reserve(m_PrimitiveStorage.size());
    for (auto& [primitiveComponentId, sceneInfo] : m_PrimitiveStorage)
    {
        (void)primitiveComponentId;
        auto* proxy = sceneInfo ? sceneInfo->GetProxy() : nullptr;
        if (proxy)
        {
            m_Primitives.push_back(proxy);
        }
    }
}

} // namespace TE
