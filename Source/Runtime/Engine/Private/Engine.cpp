// ToyEngine Engine Module
// 引擎主类实现 - UE5 架构单线程版本
// 通过 World/FScene/SceneRenderer 管理渲染

#include "Engine.h"

#include "Window.h"
#include "Memory/Memory.h"
#include "Log/Log.h"
#include "Math/ScalarMath.h"
#include "RHI.h"

// UE5 架构模块
#include "World.h"
#include "CameraComponent.h"
#include "RendererScene.h"
#include "SceneRenderer.h"
#include "InputManager.h"

namespace TE {

Engine& Engine::Get()
{
    static Engine instance;
    return instance;
}

void Engine::Init()
{
    // 1. 初始化日志（最先 — 必须在一切 TE_LOG 调用之前）
    Log::Init();

    // 防止重复初始化
    if (m_Running)
    {
        TE_LOG_WARN("Engine already initialized");
        return;
    }

    TE_LOG_INFO("Initializing ToyEngine (UE5 Architecture)...");
    TE_LOG_INFO("Log system initialized");

    // 2. 初始化内存系统
    MemoryInit();
    TE_LOG_INFO("Memory system initialized");

    // 3. 创建窗口（会同时创建 OpenGL Context 并加载 glad）
    FWindowConfig config{
        "ToyEngine - Model Loading (UE5 Architecture)",
        1280,
        720,
        true,
    };

    m_Window = IWindow::Create(config);
    if (!m_Window)
    {
        TE_LOG_ERROR("Failed to create window!");
        Shutdown();
        return;
    }

    TE_LOG_INFO("Window created: {}x{}", config.width, config.height);

    // 关闭 VSync，观察真实渲染性能（发布时建议开启）
    m_Window->SetVSync(true);

    // 4. 初始化 RHI
    if (!InitRHI())
    {
        TE_LOG_ERROR("Failed to initialize RHI!");
        Shutdown();
        return;
    }

    // 5. 创建 UE5 架构核心模块
    m_Scene = std::make_unique<FScene>(m_RHIDevice.get());
    m_SceneRenderer = std::make_unique<FSceneRenderer>();
    m_World = std::make_unique<World>();

    // 设置 World 的渲染场景接口
    m_World->SetRenderScene(m_Scene.get());

    TE_LOG_INFO("UE5 architecture modules created: World + FScene + SceneRenderer");

    // 6. 初始化输入系统
    m_InputManager = std::make_unique<FInputManager>();
    m_InputManager->Init(m_Window.get());

    // 重置时间
    m_LastFrameTime = std::chrono::high_resolution_clock::now();
    m_TotalTime = 0.0f;
    m_FrameCount = 0;
    m_DeltaTime = 0.0f;
    m_FPSAccumulatedTime = 0.0f;
    m_FPSAccumulatedFrames = 0;
    m_CurrentFPS = 0.0f;
    m_Running = true;
    m_ShouldExit = false;
    m_CameraComponent = nullptr;

    // 7. 调用应用层场景初始化回调（由 Sandbox 提供）
    if (m_SceneSetupCallback)
    {
        m_SceneSetupCallback(*this);
    }
    else
    {
        TE_LOG_WARN("Scene setup callback not set. World is empty by default.");
    }

    TE_LOG_INFO("ToyEngine initialized successfully (UE5 Architecture)");
}

bool Engine::InitRHI()
{
    TE_LOG_INFO("Initializing RHI...");

    // 1. 创建 RHI Device（工厂方法根据编译选项选择后端）
    m_RHIDevice = RHIDevice::Create();
    if (!m_RHIDevice)
    {
        TE_LOG_ERROR("Failed to create RHI Device!");
        return false;
    }

    // 2. 创建命令缓冲区
    m_CommandBuffer = m_RHIDevice->CreateCommandBuffer();
    if (!m_CommandBuffer)
    {
        TE_LOG_ERROR("Failed to create command buffer!");
        return false;
    }

    TE_LOG_INFO("RHI initialized - Device + CommandBuffer ready");
    return true;
}

void Engine::ShutdownRHI()
{
    TE_LOG_INFO("Shutting down RHI...");

    // 先销毁 UE5 架构模块（其中 Proxy 持有 RHI 资源）
    m_World.reset();
    m_SceneRenderer.reset();
    m_Scene.reset();

    // 再销毁 RHI 核心
    m_CommandBuffer.reset();
    m_RHIDevice.reset();

    m_CameraComponent = nullptr;
    m_InputManager.reset();

    TE_LOG_INFO("RHI shutdown complete");
}

void Engine::Run()
{
    if (!m_Running)
    {
        TE_LOG_ERROR("Engine not initialized! Call Init() first.");
        return;
    }

    TE_LOG_INFO("Engine main loop started (UE5 Architecture)");

    while (m_Running && !m_ShouldExit)
    {
        // 计算 delta time
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - m_LastFrameTime;
        m_DeltaTime = elapsed.count();
        m_LastFrameTime = now;

        // 限制最大 delta time（防止调试暂停时的时间跳跃）
        constexpr float MAX_DELTA_TIME = 0.1f;
        m_DeltaTime = Math::Min(m_DeltaTime, MAX_DELTA_TIME);

        m_TotalTime += m_DeltaTime;
        m_FrameCount++;

        // 检查窗口是否应该关闭
        if (m_Window && m_Window->ShouldClose())
        {
            m_ShouldExit = true;
            break;
        }

        // 执行一帧更新
        Tick(m_DeltaTime);
    }

    m_Running = false;
    TE_LOG_INFO("Engine main loop ended. Total frames: {}", m_FrameCount);
}

void Engine::Tick(float deltaTime)
{
    // ========================================
    // UE5 单线程数据流
    // ========================================

    // 1. 处理窗口事件
    if (m_Window)
    {
        m_Window->PollEvents();
    }

    // 2. World::Tick - 逻辑更新
    if (m_InputManager)
    {
        m_InputManager->Tick();
    }

    // 3. 应用层每帧逻辑回调（示例逻辑由 Sandbox 注入）
    if (m_FrameUpdateCallback)
    {
        m_FrameUpdateCallback(*this, deltaTime);
    }

    // 4. World::Tick - 逻辑更新
    if (m_World)
    {
        m_World->Tick(deltaTime);
    }

    // 5. SyncToScene - 脏 Component 同步到渲染场景接口
    if (m_World && m_Scene)
    {
        m_World->SyncToScene();
    }

    // 6. 更新 ViewInfo（从 CameraComponent 构建）
    if (m_CameraComponent && m_Scene)
    {
        // 更新视口尺寸（以防窗口大小改变）
        if (m_Window)
        {
            m_CameraComponent->SetViewportSize(
                static_cast<float>(m_Window->GetFramebufferWidth()),
                static_cast<float>(m_Window->GetFramebufferHeight())
            );
        }

        FViewInfo viewInfo = m_CameraComponent->BuildViewInfo();
        m_Scene->SetViewInfo(viewInfo);
    }

    // 7. SceneRenderer::Render - 遍历 Proxy → DrawCmd → RHI 提交
    if (m_SceneRenderer && m_Scene && m_CommandBuffer)
    {
        m_SceneRenderer->Render(m_Scene.get(), m_RHIDevice.get(), m_CommandBuffer.get());
    }

    if (m_InputManager)
    {
        m_InputManager->PostTick();
    }

    // 8. 交换缓冲区
    if (m_Window)
    {
        m_Window->SwapBuffers();
    }

    // 每 0.5 秒输出一次平均 FPS + 渲染统计（滑动窗口）
    m_FPSAccumulatedTime += deltaTime;
    m_FPSAccumulatedFrames++;
    if (m_FPSAccumulatedTime >= FPS_UPDATE_INTERVAL)
    {
        m_CurrentFPS = static_cast<float>(m_FPSAccumulatedFrames) / m_FPSAccumulatedTime;
        TE_LOG_DEBUG("FPS: {:.1f} (avg over {:.2f}s, {} frames) | DC: {} PipeBinds: {} VBOBinds: {} IBOBinds: {}",
                     m_CurrentFPS, m_FPSAccumulatedTime, m_FPSAccumulatedFrames,
                     m_SceneRenderer ? m_SceneRenderer->GetLastDrawCallCount() : 0,
                     m_SceneRenderer ? m_SceneRenderer->GetLastPipelineBindCount() : 0,
                     m_SceneRenderer ? m_SceneRenderer->GetLastVBOBindCount() : 0,
                     m_SceneRenderer ? m_SceneRenderer->GetLastIBOBindCount() : 0);
        m_FPSAccumulatedTime = 0.0f;
        m_FPSAccumulatedFrames = 0;
    }
}

void Engine::Shutdown()
{
    TE_LOG_INFO("Shutting down ToyEngine...");

    if (m_InputManager)
    {
        m_InputManager->Shutdown();
    }

    // 逆序关闭子系统
    ShutdownRHI();

    if (m_Window)
    {
        TE_LOG_INFO("Destroying window...");
        m_Window.reset();
    }

    TE_LOG_INFO("Shutting down memory system...");
    MemoryShutdown();

    TE_LOG_INFO("ToyEngine shutdown complete");

    m_Running = false;
}

void Engine::RequestExit()
{
    if (!m_ShouldExit)
    {
        TE_LOG_INFO("Exit requested");
        m_ShouldExit = true;
    }
}

void Engine::SetSceneSetupCallback(std::function<void(Engine&)> callback)
{
    m_SceneSetupCallback = std::move(callback);
}

void Engine::SetFrameUpdateCallback(std::function<void(Engine&, float)> callback)
{
    m_FrameUpdateCallback = std::move(callback);
}

} // namespace TE
