// ToyEngine Renderer Module
// FScene 实现 - 渲染场景 Proxy 管理

#include "FScene.h"
#include "FPrimitiveSceneProxy.h"
#include "Log/Log.h"

namespace TE {

FScene::~FScene() = default;

RenderPrimitiveHandle FScene::AddPrimitive(std::unique_ptr<FPrimitiveSceneProxy> proxy)
{
    if (!proxy)
    {
        TE_LOG_WARN("[Renderer] FScene::AddPrimitive called with null proxy");
        return InvalidRenderPrimitiveHandle;
    }

    const RenderPrimitiveHandle handle = m_NextHandle++;
    m_PrimitiveStorage.emplace(handle, std::move(proxy));
    RebuildPrimitiveView();
    TE_LOG_INFO("[Renderer] FScene::AddPrimitive handle={}, total primitives: {}", handle, m_PrimitiveStorage.size());
    return handle;
}

void FScene::RemovePrimitive(RenderPrimitiveHandle handle)
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
    TE_LOG_INFO("[Renderer] FScene::RemovePrimitive handle={}, total primitives: {}", handle, m_PrimitiveStorage.size());
}

void FScene::UpdatePrimitiveWorldMatrix(RenderPrimitiveHandle handle, const Matrix4& worldMatrix)
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
