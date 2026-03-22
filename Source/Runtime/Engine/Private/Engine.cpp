// ToyEngine Engine Module
// 引擎主类实现

#include "Engine.h"

#include "Window.h"
#include "Memory/Memory.h"
#include "Log/Log.h"
#include "Math/MathUtils.h"

namespace TE {

Engine& Engine::Get()
{
    static Engine instance;
    return instance;
}

void Engine::Init()
{
    // 防止重复初始化
    if (m_Running)
    {
        TE_LOG_WARN("Engine already initialized");
        return;
    }

    TE_LOG_INFO("Initializing ToyEngine...");

    // 1. 初始化日志（最先）
    Log::Init();
    TE_LOG_INFO("Log system initialized");

    // 2. 初始化内存系统
    MemoryInit();
    TE_LOG_INFO("Memory system initialized");

    // 3. 创建窗口
    WindowConfig config;
    config.title = "ToyEngine - Phase 1";
    config.width = 1280;
    config.height = 720;
    config.resizable = true;
    config.vsync = true;

    m_Window = Window::Create(config);
    if (!m_Window)
    {
        TE_LOG_ERROR("Failed to create window!");
        Shutdown();
        return;
    }

    m_Window->Show();
    TE_LOG_INFO("Window created: {}x{}", config.width, config.height);

    // 设置关闭回调
    m_Window->SetCloseCallback([this]() {
        TE_LOG_INFO("Window close requested");
        RequestExit();
    });

    // 重置时间
    m_LastFrameTime = std::chrono::high_resolution_clock::now();
    m_TotalTime = 0.0f;
    m_FrameCount = 0;
    m_DeltaTime = 0.0f;
    m_Running = true;
    m_ShouldExit = false;

    TE_LOG_INFO("ToyEngine initialized successfully");
}

void Engine::Run()
{
    if (!m_Running)
    {
        TE_LOG_ERROR("Engine not initialized! Call Init() first.");
        return;
    }

    TE_LOG_INFO("Engine main loop started");

    while (m_Running && !m_ShouldExit)
    {
        // 计算 delta time
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> elapsed = now - m_LastFrameTime;
        m_DeltaTime = elapsed.count();
        m_LastFrameTime = now;

        // 限制最大 delta time（防止在调试器等暂停时的时间跳跃）
        const float MAX_DELTA_TIME = 0.1f;  // 100ms
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
    // 1. 处理窗口事件
    if (m_Window)
    {
        m_Window->PollEvents();
    }

    // Phase 2+ 将添加：
    // 2. World->Tick(deltaTime) - 更新场景中的所有 Actor/Component
    // 3. Renderer->Render(World) - 渲染场景

    // Phase 1：简单的空循环，保持窗口响应
    // 这里可以添加简单的测试代码来验证 Math 模块

    // 示例：每 60 帧输出一次 FPS（调试用）
    if (m_FrameCount % 60 == 0 && m_FrameCount > 0)
    {
        float fps = 1.0f / deltaTime;
        // TE_LOG_DEBUG("FPS: {:.1f}, Delta: {:.3f}ms", fps, deltaTime * 1000.0f);
        (void)fps; // 避免未使用警告
    }
}

void Engine::Shutdown()
{
    TE_LOG_INFO("Shutting down ToyEngine...");

    // 逆序关闭子系统
    // 1. 关闭窗口
    if (m_Window)
    {
        TE_LOG_INFO("Destroying window...");
        m_Window.reset();
    }

    // 2. 关闭内存系统
    TE_LOG_INFO("Shutting down memory system...");
    MemoryShutdown();

    // 3. 日志系统最后关闭（因为它被用于其他系统的关闭日志）
    TE_LOG_INFO("ToyEngine shutdown complete");
    // 注意：Log 没有 Shutdown 方法，它会在程序结束时自动清理

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

} // namespace TE
