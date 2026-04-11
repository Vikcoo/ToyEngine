// ToyEngine Engine Module
// 引擎主类 - 管理所有子系统和主循环
// 重构为 UE5 架构：持有 World + FScene + SceneRenderer

#pragma once

#include "RenderSceneBridge.h"

#include <memory>
#include <chrono>
#include <functional>

// 前向声明
namespace TE {
    class Window;
    class InputManager;
    class RHIDevice;
    class RHICommandBuffer;
    class TWorld;
    class FScene;
    class SceneRenderer;
    class TCameraComponent;
}

namespace TE {

/// 引擎主类 - UE5 架构单线程版本
///
/// 数据流（每帧）：
/// Engine::Tick(deltaTime)
///   → PollEvents()
///   → 应用层 FrameUpdateCallback      // Sandbox/应用逻辑
///   → World::Tick(deltaTime)          // 逻辑更新
///   → World::SyncToScene()            // 脏 Component 同步到渲染桥接层
///   → CameraComponent::BuildViewInfo  // 构建视图信息
///   → SceneRenderer::Render(FScene)   // 遍历 Proxy → 收集 DrawCmd → RHI 提交
///   → SwapBuffers()
class Engine
{
public:
    // 禁止拷贝
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    /// 获取引擎单例
    [[nodiscard]] static Engine& Get();

    /// 初始化所有子系统
    /// 顺序：Log → Memory → Window → RHI → World/FScene/SceneRenderer → 应用层场景回调
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
    [[nodiscard]] Window* GetWindow() const { return m_Window.get(); }

    /// 获取 RHI 设备
    [[nodiscard]] RHIDevice* GetRHIDevice() const { return m_RHIDevice.get(); }
    /// 获取输入管理器
    [[nodiscard]] InputManager* GetInputManager() const { return m_InputManager.get(); }
    /// 获取游戏世界
    [[nodiscard]] TWorld* GetWorld() const { return m_World.get(); }

    /// 获取当前帧时间（秒）
    [[nodiscard]] float GetDeltaTime() const { return m_DeltaTime; }

    /// 获取总运行时间（秒）
    [[nodiscard]] float GetTotalTime() const { return m_TotalTime; }

    /// 获取当前帧数
    [[nodiscard]] uint64_t GetFrameCount() const { return m_FrameCount; }

    /// 应用层场景初始化回调（通常由 Sandbox 注册）
    void SetSceneSetupCallback(std::function<void(Engine&)> callback);
    /// 应用层每帧逻辑回调（在 World::Tick 前执行）
    void SetFrameUpdateCallback(std::function<void(Engine&, float)> callback);
    /// 设置当前主相机组件（用于构建 ViewInfo）
    void SetActiveCameraComponent(TCameraComponent* camera) { m_CameraComponent = camera; }

private:
    Engine() = default;
    ~Engine() = default;



    /// 初始化 RHI 子系统（创建 Device 和 CommandBuffer）
    [[nodiscard]] bool InitRHI();

    /// 关闭 RHI 子系统
    void ShutdownRHI();

    /// 单帧更新
    void Tick(float deltaTime);

    // Platform 子系统
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<InputManager> m_InputManager;

    // RHI 子系统（全局单例资源）
    std::unique_ptr<RHIDevice>          m_RHIDevice;
    std::unique_ptr<RHICommandBuffer>   m_CommandBuffer;

    // UE5 架构核心模块
    std::unique_ptr<TWorld>         m_World;            // 游戏世界（Actor/Component）
    std::unique_ptr<FScene>         m_Scene;            // 渲染场景（Proxy 容器）
    std::unique_ptr<IRenderSceneBridge> m_RenderBridge; // 游戏侧到渲染侧的桥接对象
    std::unique_ptr<SceneRenderer>  m_SceneRenderer;    // 渲染调度器

    // 相机组件引用（用于每帧构建 ViewInfo）
    TCameraComponent* m_CameraComponent = nullptr;
    std::function<void(Engine&)> m_SceneSetupCallback;
    std::function<void(Engine&, float)> m_FrameUpdateCallback;

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
