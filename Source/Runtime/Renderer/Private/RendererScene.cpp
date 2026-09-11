// ToyEngine Renderer Module
// FScene 实现 - 渲染场景容器与 Primitive 生命周期管理

#include "RendererScene.h"

#include "RenderResourceManager.h"
#include "StaticMeshSceneProxy.h"
#include "Log/Log.h"

#include <type_traits>
#include <utility>
#include <variant>

namespace TE {

FScene::FScene(RHIDevice* device)
    : m_RenderResourceManager(std::make_unique<FRenderResourceManager>(device))
{
}

FScene::~FScene() = default;

void FScene::ApplyCommands(std::vector<FRenderSceneCommand> commands)
{
    for (auto& command : commands)
    {
        std::visit([this](auto& typedCommand)
        {
            using TCommand = std::remove_cvref_t<decltype(typedCommand)>;
            if constexpr (std::is_same_v<TCommand, FAddPrimitiveCommand>)
            {
                (void)AddPrimitive(typedCommand.PrimitiveId, std::move(typedCommand.Proxy));
            }
            else if constexpr (std::is_same_v<TCommand, FUpdatePrimitiveTransformCommand>)
            {
                UpdatePrimitiveTransform(typedCommand.PrimitiveId, typedCommand.WorldMatrix);
            }
            else if constexpr (std::is_same_v<TCommand, FRemovePrimitiveCommand>)
            {
                RemovePrimitive(typedCommand.PrimitiveId);
            }
            else if constexpr (std::is_same_v<TCommand, FAddLightCommand>)
            {
                (void)AddLight(typedCommand.LightId, std::move(typedCommand.Proxy));
            }
            else if constexpr (std::is_same_v<TCommand, FUpdateLightCommand>)
            {
                UpdateLight(typedCommand.LightId, std::move(typedCommand.Proxy));
            }
            else if constexpr (std::is_same_v<TCommand, FRemoveLightCommand>)
            {
                RemoveLight(typedCommand.LightId);
            }
            else
            {
                static_assert(!sizeof(TCommand), "Unhandled render scene command");
            }
        }, command);
    }
}

bool FScene::AddPrimitive(FPrimitiveComponentId primitiveComponentId,
                          std::unique_ptr<FPrimitiveSceneProxy> proxy)
{
    if (!proxy || !primitiveComponentId.IsValid())
    {
        TE_LOG_WARN("[Renderer] FScene::AddPrimitive called with invalid proxy/id");
        return false;
    }

    if (!PrepareProxyResources(*proxy))
    {
        TE_LOG_WARN("[Renderer] FScene::AddPrimitive failed to prepare proxy resources");
        return false;
    }

    RemovePrimitive(primitiveComponentId);
    return InsertPrimitive(primitiveComponentId, std::move(proxy));
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
    if (m_RenderResourceManager)
    {
        m_RenderResourceManager->PurgeExpiredStaticMeshRenderData();
    }
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

bool FScene::AddLight(FLightComponentId lightComponentId,
                      std::unique_ptr<FLightSceneProxy> proxy)
{
    if (!proxy || !lightComponentId.IsValid())
    {
        TE_LOG_WARN("[Renderer] FScene::AddLight called with invalid proxy/id");
        return false;
    }

    RemoveLight(lightComponentId);
    m_LightStorage[lightComponentId] = std::move(proxy);
    RebuildLightView();
    TE_LOG_INFO("[Renderer] FScene::AddLight id={}, total lights: {}",
                lightComponentId.Value, m_LightStorage.size());
    return true;
}

void FScene::UpdateLight(FLightComponentId lightComponentId, std::unique_ptr<FLightSceneProxy> proxy)
{
    if (!lightComponentId.IsValid() || !proxy)
    {
        return;
    }

    const auto it = m_LightStorage.find(lightComponentId);
    if (it == m_LightStorage.end())
    {
        return;
    }

    it->second = std::move(proxy);
    RebuildLightView();
}

void FScene::RemoveLight(FLightComponentId lightComponentId)
{
    if (!lightComponentId.IsValid())
    {
        return;
    }

    const auto it = m_LightStorage.find(lightComponentId);
    if (it == m_LightStorage.end())
    {
        return;
    }

    m_LightStorage.erase(it);
    RebuildLightView();
    TE_LOG_INFO("[Renderer] FScene::RemoveLight id={}, total lights: {}",
                lightComponentId.Value, m_LightStorage.size());
}

RHIPipeline* FScene::ResolvePreparedPipeline(const FPipelineKey& pipelineKey) const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetPreparedPipeline(pipelineKey);
}

RHITexture* FScene::ResolvePreparedBaseColorTexture(const StaticMesh* staticMesh, uint32_t materialIndex) const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetPreparedBaseColorTexture(staticMesh, materialIndex);
}

const FPreparedMaterialTextures* FScene::ResolvePreparedMaterialTextures(const StaticMesh* staticMesh,
                                                                         uint32_t materialIndex) const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetPreparedMaterialTextures(staticMesh, materialIndex);
}

const FMaterial* FScene::ResolveMaterial(const StaticMesh* staticMesh, uint32_t materialIndex) const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetMaterial(staticMesh, materialIndex);
}

const FEnvironmentIBLResources* FScene::ResolveEnvironmentIBLResources() const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetEnvironmentIBLResources();
}

RHISampler* FScene::ResolveDefaultSampler() const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetDefaultSampler();
}

RHISampler* FScene::ResolveEnvironmentSampler() const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetEnvironmentSampler();
}

RHISampler* FScene::ResolveGBufferSampler() const
{
    if (!m_RenderResourceManager)
    {
        return nullptr;
    }
    return m_RenderResourceManager->GetGBufferSampler();
}

bool FScene::PrepareProxyResources(FPrimitiveSceneProxy& proxy)
{
    auto* staticMeshProxy = dynamic_cast<FStaticMeshSceneProxy*>(&proxy);
    if (!staticMeshProxy)
    {
        return true;
    }

    if (!m_RenderResourceManager)
    {
        return false;
    }

    if (!m_RenderResourceManager->PrepareStaticMeshProxy(*staticMeshProxy))
    {
        TE_LOG_WARN("[Renderer] FScene::PrepareProxyResources failed for FStaticMeshSceneProxy");
        return false;
    }

    return true;
}

bool FScene::InsertPrimitive(FPrimitiveComponentId primitiveComponentId,
                             std::unique_ptr<FPrimitiveSceneProxy> proxy)
{
    if (!proxy || !primitiveComponentId.IsValid())
    {
        TE_LOG_WARN("[Renderer] FScene::InsertPrimitive called with invalid proxy/id");
        return false;
    }

    auto sceneInfo = std::make_unique<FPrimitiveSceneInfo>(primitiveComponentId, std::move(proxy));
    m_PrimitiveStorage[primitiveComponentId] = std::move(sceneInfo);
    RebuildPrimitiveView();
    TE_LOG_INFO("[Renderer] FScene::InsertPrimitive id={}, total primitives: {}",
                primitiveComponentId.Value, m_PrimitiveStorage.size());
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

void FScene::RebuildLightView()
{
    m_Lights.clear();
    m_Lights.reserve(m_LightStorage.size());
    for (auto& [lightComponentId, lightProxy] : m_LightStorage)
    {
        (void)lightComponentId;
        if (lightProxy)
        {
            m_Lights.push_back(lightProxy.get());
        }
    }
}

} // namespace TE
