// ToyEngine Engine Module
// 引擎主类 - 管理所有子系统和主循环
// 重构为 UE5 架构：持有 World + FScene + SceneRenderer

#pragma once

#include "RenderPathTypes.h"

#include <memory>
#include <chrono>
#include <functional>

// 前向声明
namespace TE {
    class IWindow;
    class FInputManager;
    class RHIDevice;
    class RHICommandBuffer;
    class World;
    class FScene;
    class FSceneRenderer;
    class CameraComponent;
}

namespace TE {

/// 引擎主类 - UE5 架构单线程版本
///
/// 单线程帧管线（每帧）：
/// Engine::Tick(deltaTime)
///   → PumpPlatformMessages()
///   → TickInput(deltaTime)
///   → TickGameThread(deltaTime)       // 应用层逻辑 + World Tick
///   → SendAllEndOfFrameUpdates()      // 游戏侧状态同步到渲染侧
///   → TickRenderThread(deltaTime)     // 当前仍在主线程中模拟渲染阶段
///   → EndFrame(deltaTime)             // 输入收尾、统计
class Engine
{
public:
    // 禁止拷贝
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// 获取引擎单例
    [[nodiscard]] static Engine& Get();

    /**
     * 初始化所有子系统。
     * @note 顺序为 Log → Memory → Window → RHI → World/FScene/SceneRenderer；仅完整场景后端执行应用层场景回调。
     */
    void Init();

    /// 运行主循环（阻塞直到退出）
    void Run();

    /// 关闭引擎，清理所有子系统
    void Shutdown();

    /// 请求退出主循环
    void RequestExit();

    /// 检查是否正在运行
    [[nodiscard]] bool IsRunning() const { return m_Running; }

    /// 获取窗口
    [[nodiscard]] IWindow* GetWindow() const { return m_Window.get(); }

    /// 获取 RHI 设备
    [[nodiscard]] RHIDevice* GetRHIDevice() const { return m_RHIDevice.get(); }
    /// 获取输入管理器
    [[nodiscard]] FInputManager* GetInputManager() const { return m_InputManager.get(); }
    /// 获取游戏世界
    [[nodiscard]] World* GetWorld() const { return m_World.get(); }

    /// 获取当前帧时间（秒）
    [[nodiscard]] float GetDeltaTime() const { return m_DeltaTime; }

    /// 获取总运行时间（秒）
    [[nodiscard]] float GetTotalTime() const { return m_TotalTime; }

    /// 获取当前帧数
    [[nodiscard]] uint64_t GetFrameCount() const { return m_FrameCount; }

    /** 注册应用层场景初始化回调；仅 `bSupportsFullSceneRendering` 后端执行。 */
    void SetSceneSetupCallback(std::function<void(Engine&)> callback);
    /** 注册应用层每帧逻辑回调；仅完整场景后端在 World::Tick 前执行。 */
    void SetFrameUpdateCallback(std::function<void(Engine&, float)> callback);
    /// 设置当前主相机组件（用于构建 ViewInfo）
    void SetActiveCameraComponent(CameraComponent* camera) { m_CameraComponent = camera; }
    void SetRenderPath(ERenderPathType type);
    [[nodiscard]] ERenderPathType GetRenderPath() const { return m_RenderPathType; }
    void SetRenderDebugView(ERenderDebugView mode);
    [[nodiscard]] ERenderDebugView GetRenderDebugView() const { return m_RenderDebugViewMode; }

private:
    Engine() = default;
    ~Engine() = default;



    /// 初始化 RHI 子系统；帧 CommandBuffer 由 Device 提供
    [[nodiscard]] bool InitRHI();

    /// 关闭 RHI 子系统
    void ShutdownRHI();

    /// 单帧更新
    void Tick(float deltaTime);
    void PumpPlatformMessages() const;
    void TickInput(float deltaTime) const;
    void TickGameThread(float deltaTime);
    void SendAllEndOfFrameUpdates() const;
    void TickRenderThread(float deltaTime) const;
    void EndFrame(float deltaTime);
    void UpdateFrameStats(float deltaTime);

    // Platform 子系统
    std::unique_ptr<IWindow> m_Window;
    std::unique_ptr<FInputManager> m_InputManager;

    // RHI 子系统（全局单例资源）
    std::unique_ptr<RHIDevice> m_RHIDevice;

    // UE5 架构核心模块
    std::unique_ptr<World>         m_World;            // 游戏世界（Actor/Component）
    std::unique_ptr<FScene>         m_Scene;            // 渲染场景（Proxy 容器）
    std::unique_ptr<FSceneRenderer>  m_SceneRenderer;    // 渲染调度器

    // 相机组件引用（用于每帧构建 ViewInfo）
    CameraComponent* m_CameraComponent = nullptr;
    std::function<void(Engine&)> m_SceneSetupCallback;
    std::function<void(Engine&, float)> m_FrameUpdateCallback;
    ERenderPathType m_RenderPathType = ERenderPathType::Forward;
    ERenderDebugView m_RenderDebugViewMode = ERenderDebugView::Lit;

    // 运行状态
    bool m_Running = false;
    bool m_ShouldExit = false;

    // 时间
    float m_DeltaTime = 0.0f;
    float m_TotalTime = 0.0f;
    uint64_t m_FrameCount = 0;

    // 用于计算 delta time
    std::chrono::high_resolution_clock::time_point m_LastFrameTime;

    // FPS 统计（滑动窗口均值）
    float m_FPSAccumulatedTime = 0.0f;   // 窗口内累计时间
    uint32_t m_FPSAccumulatedFrames = 0; // 窗口内累计帧数
    float m_CurrentFPS = 0.0f;           // 最近一次计算出的平均 FPS
    static constexpr float FPS_UPDATE_INTERVAL = 0.5f; // 每 0.5 秒更新一次
};

} // namespace TE
