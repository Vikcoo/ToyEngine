// ToyEngine Engine Module
// 引擎主类 - 管理所有子系统和主循环

#pragma once

#include <memory>
#include <chrono>

// 前向声明
namespace TE {
    class Window;
}

namespace TE {

/// <summary>
/// 引擎主类 - Phase 1 精简版
/// 当前仅管理 Core 和 Platform 子系统
/// Phase 2+ 将添加 RHI、AssetManager、World、Renderer 等
/// </summary>
class Engine
{
public:
    /// <summary>
    /// 获取引擎单例
    /// </summary>
    static Engine& Get();

    /// <summary>
    /// 初始化所有子系统
    /// 顺序：Log → Memory → Window
    /// </summary>
    void Init();

    /// <summary>
    /// 运行主循环（阻塞直到退出）
    /// </summary>
    void Run();

    /// <summary>
    /// 关闭引擎，清理所有子系统
    /// 顺序（逆序）：Window → Memory
    /// </summary>
    void Shutdown();

    /// <summary>
    /// 请求退出主循环
    /// </summary>
    void RequestExit();

    /// <summary>
    /// 检查是否正在运行
    /// </summary>
    bool IsRunning() const { return m_Running; }

    /// <summary>
    /// 获取窗口
    /// </summary>
    Window* GetWindow() const { return m_Window.get(); }

    /// <summary>
    /// 获取当前帧时间（秒）
    /// </summary>
    float GetDeltaTime() const { return m_DeltaTime; }

    /// <summary>
    /// 获取总运行时间（秒）
    /// </summary>
    float GetTotalTime() const { return m_TotalTime; }

    /// <summary>
    /// 获取当前帧数
    /// </summary>
    uint64_t GetFrameCount() const { return m_FrameCount; }

private:
    Engine() = default;
    ~Engine() = default;

    // 禁止拷贝
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// <summary>
    /// 单帧更新
    /// </summary>
    void Tick(float deltaTime);

    // Phase 1 子系统
    std::unique_ptr<Window> m_Window;

    // 运行状态
    bool m_Running = false;
    bool m_ShouldExit = false;

    // 时间
    float m_DeltaTime = 0.0f;
    float m_TotalTime = 0.0f;
    uint64_t m_FrameCount = 0;

    // 用于计算 delta time
    std::chrono::high_resolution_clock::time_point m_LastFrameTime;
};

} // namespace TE
