// ToyEngine Engine Module
// 引擎主类实现 - UE5 架构单线程版本
// 通过 World/FScene/SceneRenderer 管理渲染

#include "Engine.h"

#include "Window.h"
#include "Memory/Memory.h"
#include "Log/Log.h"
#include "Math/MathUtils.h"
#include "Math/MathTypes.h"
#include "RHI.h"

// UE5 架构模块
#include "World.h"
#include "Actor.h"
#include "MeshComponent.h"
#include "CameraComponent.h"
#include "PrimitiveComponent.h"
#include "FScene.h"
#include "SceneRenderer.h"
#include "FStaticMeshSceneProxy.h"

#include <string>

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

    TE_LOG_INFO("Initializing ToyEngine (UE5 Architecture)...");

    // 1. 初始化日志（最先）
    Log::Init();
    TE_LOG_INFO("Log system initialized");

    // 2. 初始化内存系统
    MemoryInit();
    TE_LOG_INFO("Memory system initialized");

    // 3. 创建窗口（会同时创建 OpenGL Context 并加载 glad）
    WindowConfig config;
    config.title = "ToyEngine - UE5 Architecture Cube";
    config.width = 1280;
    config.height = 720;
    config.resizable = true;

    m_Window = Window::Create(config);
    if (!m_Window)
    {
        TE_LOG_ERROR("Failed to create window!");
        Shutdown();
        return;
    }

    TE_LOG_INFO("Window created: {}x{}", config.width, config.height);

    // 关闭 VSync，观察真实渲染性能（发布时建议开启）
    m_Window->SetVSync(false);

    // 4. 初始化 RHI
    if (!InitRHI())
    {
        TE_LOG_ERROR("Failed to initialize RHI!");
        Shutdown();
        return;
    }

    // 5. 创建 UE5 架构核心模块
    m_Scene = std::make_unique<FScene>();
    m_SceneRenderer = std::make_unique<SceneRenderer>();
    m_World = std::make_unique<TWorld>();

    // 设置 World 的渲染场景和设备引用
    m_World->SetScene(m_Scene.get());
    m_World->SetRHIDevice(m_RHIDevice.get());

    TE_LOG_INFO("UE5 architecture modules created: World + FScene + SceneRenderer");

    // 6. 构建游戏场景
    BuildScene();

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

void Engine::BuildScene()
{
    TE_LOG_INFO("Building game scene (UE5 Architecture)...");

    // ========================================
    // 创建立方体 Actor
    // ========================================
    auto cubeActor = std::make_unique<TActor>();
    cubeActor->SetName("CubeActor");

    // 添加 MeshComponent（网格组件）
    auto* meshComp = cubeActor->AddComponent<TMeshComponent>();
    meshComp->SetName("CubeMesh");

    // 准备立方体网格数据
    FStaticMeshData meshData;

    // 立方体 24 个顶点（每面 4 个独立顶点，每面不同颜色）
    // 顶点布局：Position(vec3) + Color(vec3) = stride 24 bytes
    // clang-format off
    meshData.Vertices = {
        // === 前面 (Z+) - 红色 ===
        -0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.2f,  // 0: 左下
         0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.2f,  // 1: 右下
         0.5f,  0.5f,  0.5f,  0.9f, 0.2f, 0.2f,  // 2: 右上
        -0.5f,  0.5f,  0.5f,  0.9f, 0.2f, 0.2f,  // 3: 左上

        // === 后面 (Z-) - 绿色 ===
         0.5f, -0.5f, -0.5f,  0.2f, 0.9f, 0.2f,  // 4: 左下（从后面看）
        -0.5f, -0.5f, -0.5f,  0.2f, 0.9f, 0.2f,  // 5: 右下
        -0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.2f,  // 6: 右上
         0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.2f,  // 7: 左上

        // === 右面 (X+) - 蓝色 ===
         0.5f, -0.5f,  0.5f,  0.2f, 0.2f, 0.9f,  // 8
         0.5f, -0.5f, -0.5f,  0.2f, 0.2f, 0.9f,  // 9
         0.5f,  0.5f, -0.5f,  0.2f, 0.2f, 0.9f,  // 10
         0.5f,  0.5f,  0.5f,  0.2f, 0.2f, 0.9f,  // 11

        // === 左面 (X-) - 黄色 ===
        -0.5f, -0.5f, -0.5f,  0.9f, 0.9f, 0.2f,  // 12
        -0.5f, -0.5f,  0.5f,  0.9f, 0.9f, 0.2f,  // 13
        -0.5f,  0.5f,  0.5f,  0.9f, 0.9f, 0.2f,  // 14
        -0.5f,  0.5f, -0.5f,  0.9f, 0.9f, 0.2f,  // 15

        // === 顶面 (Y+) - 青色 ===
        -0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f,  // 16
         0.5f,  0.5f,  0.5f,  0.2f, 0.9f, 0.9f,  // 17
         0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f,  // 18
        -0.5f,  0.5f, -0.5f,  0.2f, 0.9f, 0.9f,  // 19

        // === 底面 (Y-) - 品红 ===
        -0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,  // 20
         0.5f, -0.5f, -0.5f,  0.9f, 0.2f, 0.9f,  // 21
         0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,  // 22
        -0.5f, -0.5f,  0.5f,  0.9f, 0.2f, 0.9f,  // 23
    };
    // clang-format on

    // 36 个索引（6 面 × 2 三角形 × 3 顶点）
    // 每个面的两个三角形，顶点顺序为 CCW（逆时针 = 正面）
    meshData.Indices = {
        // 前面
         0,  1,  2,   2,  3,  0,
        // 后面
         4,  5,  6,   6,  7,  4,
        // 右面
         8,  9, 10,  10, 11,  8,
        // 左面
        12, 13, 14,  14, 15, 12,
        // 顶面
        16, 17, 18,  18, 19, 16,
        // 底面
        20, 21, 22,  22, 23, 20,
    };

    meshData.VertexStride = 6 * sizeof(float);  // Position(3) + Color(3)

    // Shader 路径
    std::string shaderDir = std::string(TE_PROJECT_ROOT_DIR) + "Content/Shaders/OpenGL/";
    meshData.VertexShaderPath = shaderDir + "cube.vert";
    meshData.FragmentShaderPath = shaderDir + "cube.frag";

    // 顶点属性布局
    meshData.Attributes = {
        { 0, static_cast<uint32_t>(RHIFormat::Float3), 0 },                      // Position: location=0
        { 1, static_cast<uint32_t>(RHIFormat::Float3), 3 * sizeof(float) },      // Color:    location=1
    };

    // Pipeline 状态
    meshData.DepthTestEnabled = true;
    meshData.DepthWriteEnabled = true;
    meshData.BackfaceCulling = true;

    meshComp->SetMeshData(meshData);

    // ========================================
    // 创建相机 Actor
    // ========================================
    auto cameraActor = std::make_unique<TActor>();
    cameraActor->SetName("CameraActor");

    auto* cameraComp = cameraActor->AddComponent<TCameraComponent>();
    cameraComp->SetName("MainCamera");
    cameraComp->SetFOV(60.0f);
    cameraComp->SetNearPlane(0.1f);
    cameraComp->SetFarPlane(100.0f);

    // 设置相机位置和视口
    if (m_Window)
    {
        cameraComp->SetViewportSize(
            static_cast<float>(m_Window->GetFramebufferWidth()),
            static_cast<float>(m_Window->GetFramebufferHeight())
        );
    }

    // 相机位置：在立方体右上前方观察
    cameraComp->SetPosition(Vector3(2.0f, 2.0f, 3.0f));
    // 朝向原点
    cameraComp->GetTransform().LookAt(Vector3::Zero);

    // 保存相机组件引用
    m_CameraComponent = cameraComp;

    // ========================================
    // 将 Actor 添加到 World
    // ========================================
    m_World->AddActor(std::move(cubeActor));
    m_World->AddActor(std::move(cameraActor));

    TE_LOG_INFO("Game scene built: CubeActor + CameraActor");
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
        const float MAX_DELTA_TIME = 0.1f;
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
    // 这里我们让立方体每帧旋转
    if (m_World)
    {
        // 获取立方体 Actor 并旋转它
        const auto& actors = m_World->GetActors();
        for (const auto& actor : actors)
        {
            if (actor->GetName() == "CubeActor")
            {
                // 每秒绕 Y 轴旋转 45 度，绕 X 轴旋转 30 度
                float rotY = Math::DegToRad(45.0f) * deltaTime;
                float rotX = Math::DegToRad(30.0f) * deltaTime;

                actor->GetTransform().RotateWorldY(rotY);
                actor->GetTransform().RotateWorldX(rotX);

                // 遍历组件，找到 PrimitiveComponent 标记脏
                for (const auto& comp : actor->GetComponents())
                {
                    if (auto* primComp = dynamic_cast<TPrimitiveComponent*>(comp.get()))
                    {
                        primComp->MarkRenderStateDirty();
                    }
                }
            }
        }

        m_World->Tick(deltaTime);
    }

    // 3. SyncToScene - 脏 Component 同步到 Proxy
    if (m_World && m_Scene)
    {
        m_World->SyncToScene(m_Scene.get());
    }

    // 4. 更新 ViewInfo（从 CameraComponent 构建）
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

    // 5. SceneRenderer::Render - 遍历 Proxy → DrawCmd → RHI 提交
    if (m_SceneRenderer && m_Scene && m_CommandBuffer)
    {
        m_SceneRenderer->Render(m_Scene.get(), m_RHIDevice.get(), m_CommandBuffer.get());
    }

    // 6. 交换缓冲区
    if (m_Window)
    {
        m_Window->SwapBuffers();
    }

    // 每 0.5 秒输出一次平均 FPS（滑动窗口）
    m_FPSAccumulatedTime += deltaTime;
    m_FPSAccumulatedFrames++;
    if (m_FPSAccumulatedTime >= FPS_UPDATE_INTERVAL)
    {
        m_CurrentFPS = static_cast<float>(m_FPSAccumulatedFrames) / m_FPSAccumulatedTime;
        TE_LOG_DEBUG("FPS: {:.1f} (avg over {:.2f}s, {} frames)",
                     m_CurrentFPS, m_FPSAccumulatedTime, m_FPSAccumulatedFrames);
        m_FPSAccumulatedTime = 0.0f;
        m_FPSAccumulatedFrames = 0;
    }
}

void Engine::Shutdown()
{
    TE_LOG_INFO("Shutting down ToyEngine...");

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

} // namespace TE
