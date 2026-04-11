// ToyEngine Renderer Module
// FScene 实现 - 渲染场景与资产级渲染资源管理

#include "FScene.h"

#include "FStaticMeshSceneProxy.h"
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

RenderPrimitiveHandle FScene::CreatePrimitive(const RenderPrimitiveCreateInfo& createInfo)
{
    if (!m_Device)
    {
        TE_LOG_ERROR("[Renderer] FScene::CreatePrimitive called with null RHIDevice");
        return InvalidRenderPrimitiveHandle;
    }

    switch (createInfo.Kind)
    {
    case RenderPrimitiveKind::StaticMesh:
    {
        if (!createInfo.StaticMesh || !createInfo.StaticMesh->IsValid())
        {
            TE_LOG_WARN("[Renderer] FScene::CreatePrimitive static mesh create info is invalid");
            return InvalidRenderPrimitiveHandle;
        }

        if (!EnsureStaticMeshPipeline())
        {
            TE_LOG_ERROR("[Renderer] FScene failed to initialize static mesh pipeline");
            return InvalidRenderPrimitiveHandle;
        }

        auto renderData = GetOrCreateStaticMeshRenderData(createInfo.StaticMesh);
        if (!renderData || !renderData->IsValid())
        {
            TE_LOG_WARN("[Renderer] FScene failed to create static mesh render data");
            return InvalidRenderPrimitiveHandle;
        }

        auto proxy = std::make_unique<FStaticMeshSceneProxy>(std::move(renderData), m_StaticMeshPipeline.get());
        if (!proxy->IsValid())
        {
            TE_LOG_WARN("[Renderer] FScene failed to create valid static mesh scene proxy");
            return InvalidRenderPrimitiveHandle;
        }

        proxy->SetWorldMatrix(createInfo.WorldMatrix);
        return InsertPrimitive(std::move(proxy));
    }
    default:
        TE_LOG_WARN("[Renderer] FScene::CreatePrimitive unsupported primitive kind");
        return InvalidRenderPrimitiveHandle;
    }
}

void FScene::DestroyPrimitive(RenderPrimitiveHandle handle)
{
    if (handle == InvalidRenderPrimitiveHandle)
    {
        return;
    }

    const auto it = m_PrimitiveStorage.find(handle);
    if (it == m_PrimitiveStorage.end())
    {
        return;
    }

    m_PrimitiveStorage.erase(it);
    RebuildPrimitiveView();
    TE_LOG_INFO("[Renderer] FScene::DestroyPrimitive handle={}, total primitives: {}", handle, m_PrimitiveStorage.size());
}

void FScene::UpdatePrimitiveTransform(RenderPrimitiveHandle handle, const Matrix4& worldMatrix)
{
    if (handle == InvalidRenderPrimitiveHandle)
    {
        return;
    }

    const auto it = m_PrimitiveStorage.find(handle);
    if (it == m_PrimitiveStorage.end())
    {
        return;
    }

    it->second->SetWorldMatrix(worldMatrix);
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

std::shared_ptr<const FStaticMeshRenderData> FScene::GetOrCreateStaticMeshRenderData(const std::shared_ptr<TStaticMesh>& staticMesh)
{
    if (!m_Device || !staticMesh)
    {
        return nullptr;
    }

    const TStaticMesh* key = staticMesh.get();
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

RenderPrimitiveHandle FScene::InsertPrimitive(std::unique_ptr<FPrimitiveSceneProxy> proxy)
{
    if (!proxy)
    {
        TE_LOG_WARN("[Renderer] FScene::InsertPrimitive called with null proxy");
        return InvalidRenderPrimitiveHandle;
    }

    const RenderPrimitiveHandle handle = m_NextHandle++;
    m_PrimitiveStorage.emplace(handle, std::move(proxy));
    RebuildPrimitiveView();
    TE_LOG_INFO("[Renderer] FScene::InsertPrimitive handle={}, total primitives: {}", handle, m_PrimitiveStorage.size());
    return handle;
}

void FScene::RebuildPrimitiveView()
{
    m_Primitives.clear();
    m_Primitives.reserve(m_PrimitiveStorage.size());
    for (auto& [handle, proxy] : m_PrimitiveStorage)
    {
        (void)handle;
        m_Primitives.push_back(proxy.get());
    }
}

} // namespace TE
