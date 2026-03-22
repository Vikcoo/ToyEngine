// ToyEngine Renderer Module
// FScene 实现 - 渲染场景 Proxy 管理

#include "FScene.h"
#include "FPrimitiveSceneProxy.h"
#include "Log/Log.h"
#include <algorithm>

namespace TE {

void FScene::AddPrimitive(FPrimitiveSceneProxy* proxy)
{
    if (!proxy)
    {
        TE_LOG_WARN("[Renderer] FScene::AddPrimitive called with null proxy");
        return;
    }

    // 检查是否已存在（避免重复添加）
    auto it = std::find(m_Primitives.begin(), m_Primitives.end(), proxy);
    if (it != m_Primitives.end())
    {
        TE_LOG_WARN("[Renderer] FScene::AddPrimitive proxy already exists");
        return;
    }

    m_Primitives.push_back(proxy);
    TE_LOG_INFO("[Renderer] FScene::AddPrimitive, total primitives: {}", m_Primitives.size());
}

void FScene::RemovePrimitive(FPrimitiveSceneProxy* proxy)
{
    auto it = std::find(m_Primitives.begin(), m_Primitives.end(), proxy);
    if (it != m_Primitives.end())
    {
        m_Primitives.erase(it);
        TE_LOG_INFO("[Renderer] FScene::RemovePrimitive, total primitives: {}", m_Primitives.size());
    }
}

} // namespace TE
