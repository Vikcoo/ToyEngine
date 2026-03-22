// ToyEngine Scene Module
// TPrimitiveComponent 实现
// 核心桥接：RegisterToScene / UnregisterFromScene / MarkRenderStateDirty

#include "PrimitiveComponent.h"
#include "FScene.h"
#include "FPrimitiveSceneProxy.h"
#include "Log/Log.h"

namespace TE {

TPrimitiveComponent::~TPrimitiveComponent()
{
    // 析构时清理 Proxy
    delete m_SceneProxy;
    m_SceneProxy = nullptr;
}

void TPrimitiveComponent::RegisterToScene(FScene* scene, RHIDevice* device)
{
    if (!scene)
    {
        TE_LOG_WARN("[Scene] RegisterToScene called with null FScene");
        return;
    }

    // 如果已有 Proxy，先注销
    if (m_SceneProxy)
    {
        UnregisterFromScene(scene);
    }

    // 调用子类 override 的 CreateSceneProxy() 创建渲染镜像
    m_SceneProxy = CreateSceneProxy(device);
    if (m_SceneProxy)
    {
        // 初始化 Proxy 的 WorldMatrix
        m_SceneProxy->SetWorldMatrix(GetWorldMatrix());

        // 加入渲染场景
        scene->AddPrimitive(m_SceneProxy);

        TE_LOG_INFO("[Scene] TPrimitiveComponent registered to FScene");
    }
    else
    {
        TE_LOG_WARN("[Scene] CreateSceneProxy returned null");
    }
}

void TPrimitiveComponent::UnregisterFromScene(FScene* scene)
{
    if (m_SceneProxy && scene)
    {
        scene->RemovePrimitive(m_SceneProxy);
        delete m_SceneProxy;
        m_SceneProxy = nullptr;
        TE_LOG_INFO("[Scene] TPrimitiveComponent unregistered from FScene");
    }
}

} // namespace TE
