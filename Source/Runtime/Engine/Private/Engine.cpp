// ToyEngine Engine Module
// 引擎主类实现 - UE5 架构单线程版本
// 通过 World/FScene/SceneRenderer 管理渲染

#include "Engine.h"

#include "Window.h"
#include "Memory/Memory.h"
#include "Log/Log.h"
#include "Math/ScalarMath.h"
#include "Math/MathTypes.h"
#include "RHI.h"

// UE5 架构模块
#include "World.h"
#include "Actor.h"
#include "MeshComponent.h"
#include "CameraComponent.h"
#include "FlyCameraController.h"
#include "PrimitiveComponent.h"
#include "FScene.h"
#include "SceneRenderer.h"
#include "InputManager.h"

// Asset 模块
#include "TStaticMesh.h"
#include "FAssetImporter.h"

#include <string>

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
    WindowConfig config{
        "ToyEngine - Model Loading (UE5 Architecture)",
        1280,
        720,
        true,
    };

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

    // 6. 初始化输入系统
    m_InputManager = std::make_unique<InputManager>();
    m_InputManager->Init(m_Window.get());

    // 7. 构建游戏场景
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
    // 加载模型资产（通过 FAssetImporter）
    // ========================================
    std::string modelDir = std::string(TE_PROJECT_ROOT_DIR) + "Content/Models/";

    // 尝试加载外部模型文件
    // 优先查找常见格式的测试模型
    std::vector<std::string> candidateFiles = {
        modelDir + "model.obj",
        modelDir + "model.fbx",
        modelDir + "model.gltf",
        modelDir + "model.glb",
    };

    for (const auto& candidate : candidateFiles)
    {
        m_LoadedMesh = FAssetImporter::ImportStaticMesh(candidate);
        if (m_LoadedMesh)
        {
            TE_LOG_INFO("Loaded model from: {}", candidate);
            break;
        }
    }

    // 如果没有找到外部模型文件，使用内置的立方体作为 fallback
    if (!m_LoadedMesh)
    {
        TE_LOG_INFO("No external model found, creating default cube mesh...");
        m_LoadedMesh = CreateDefaultCubeMesh();
    }

    // ========================================
    // 创建模型 Actor
    // ========================================
    auto meshActor = std::make_unique<TActor>();
    meshActor->SetName("MeshActor");

    // 添加 MeshComponent，引用加载的 TStaticMesh 资产
    auto* meshComp = meshActor->AddComponent<TMeshComponent>();
    meshComp->SetName("ModelMesh");
    meshComp->SetStaticMesh(m_LoadedMesh);

    // ========================================
    // 创建第二个模型 Actor（位于主模型上方，缩放为 0.5）
    // ========================================
    auto meshActorTop = std::make_unique<TActor>();
    meshActorTop->SetName("MeshActorTop");

    auto* meshCompTop = meshActorTop->AddComponent<TMeshComponent>();
    meshCompTop->SetName("ModelMeshTop");
    meshCompTop->SetStaticMesh(m_LoadedMesh);
    meshCompTop->SetScale(Vector3(0.5f, 0.5f, 0.5f));
    meshCompTop->SetPosition(Vector3(0.0f, 1.5f, 0.0f));

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

    // 相机位置：在模型右上前方观察
    cameraComp->SetPosition(Vector3(0.0f, 0.0f, 3.0f));
    // 朝向原点
    cameraComp->LookAt(Vector3::Zero);

    // 保存相机组件引用
    m_CameraComponent = cameraComp;

    // FlyCamera 控制器（默认视口）
    auto* flyCamCtrl = cameraActor->AddComponent<TFlyCameraController>();
    flyCamCtrl->SetName("FlyCameraController");
    flyCamCtrl->SetInputManager(m_InputManager.get());
    flyCamCtrl->SetWindow(m_Window.get());

    // ========================================
    // 将 Actor 添加到 World
    // ========================================
    m_World->AddActor(std::move(meshActor));
    m_World->AddActor(std::move(meshActorTop));
    m_World->AddActor(std::move(cameraActor));

    TE_LOG_INFO("Game scene built: MeshActor + MeshActorTop ({}) + CameraActor",
                m_LoadedMesh ? m_LoadedMesh->GetName() : "default_cube");
}

std::shared_ptr<TStaticMesh> Engine::CreateDefaultCubeMesh()
{
    auto mesh = std::make_shared<TStaticMesh>();
    mesh->SetName("DefaultCube");

    FMeshSection section;

    // 立方体 24 个顶点（每面 4 个独立顶点，每面不同颜色）
    // 前面 (Z+) - 红色
    section.Vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}, {0.9f, 0.2f, 0.2f}});
    section.Vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}, {0.9f, 0.2f, 0.2f}});
    section.Vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}, {0.9f, 0.2f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}, {0.9f, 0.2f, 0.2f}});

    // 后面 (Z-) - 绿色
    section.Vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}, {0.2f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}, {0.2f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}, {0.2f, 0.9f, 0.2f}});
    section.Vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}, {0.2f, 0.9f, 0.2f}});

    // 右面 (X+) - 蓝色
    section.Vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.2f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.2f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.2f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.2f, 0.2f, 0.9f}});

    // 左面 (X-) - 黄色
    section.Vertices.push_back({{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}, {0.9f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}, {0.9f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}, {0.9f, 0.9f, 0.2f}});
    section.Vertices.push_back({{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}, {0.9f, 0.9f, 0.2f}});

    // 顶面 (Y+) - 青色
    section.Vertices.push_back({{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}, {0.2f, 0.9f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}, {0.2f, 0.9f, 0.9f}});
    section.Vertices.push_back({{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}, {0.2f, 0.9f, 0.9f}});
    section.Vertices.push_back({{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}, {0.2f, 0.9f, 0.9f}});

    // 底面 (Y-) - 品红
    section.Vertices.push_back({{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}, {0.9f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}, {0.9f, 0.2f, 0.9f}});
    section.Vertices.push_back({{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}, {0.9f, 0.2f, 0.9f}});
    section.Vertices.push_back({{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}, {0.9f, 0.2f, 0.9f}});

    // 36 个索引（6 面 × 2 三角形 × 3 顶点，CCW 正面）
    section.Indices = {
         0,  1,  2,   2,  3,  0,    // 前面
         4,  5,  6,   6,  7,  4,    // 后面
         8,  9, 10,  10, 11,  8,    // 右面
        12, 13, 14,  14, 15, 12,    // 左面
        16, 17, 18,  18, 19, 16,    // 顶面
        20, 21, 22,  22, 23, 20,    // 底面
    };

    mesh->AddSection(std::move(section));
    return mesh;
}

void Engine::ShutdownRHI()
{
    TE_LOG_INFO("Shutting down RHI...");

    // 先销毁 UE5 架构模块（其中 Proxy 持有 RHI 资源）
    m_World.reset();
    m_SceneRenderer.reset();
    m_Scene.reset();

    // 释放资产
    m_LoadedMesh.reset();

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

    // 3. World::Tick - 逻辑更新
    // 让模型每帧旋转
    if (m_World)
    {
        const auto& actors = m_World->GetActors();
        for (const auto& actor : actors)
        {
            if (actor->GetName() == "MeshActor")
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

    // 4. SyncToScene - 脏 Component 同步到 Proxy
    if (m_World && m_Scene)
    {
        m_World->SyncToScene(m_Scene.get());
    }

    // 5. 更新 ViewInfo（从 CameraComponent 构建）
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

    // 6. SceneRenderer::Render - 遍历 Proxy → DrawCmd → RHI 提交
    if (m_SceneRenderer && m_Scene && m_CommandBuffer)
    {
        m_SceneRenderer->Render(m_Scene.get(), m_RHIDevice.get(), m_CommandBuffer.get());
    }

    if (m_InputManager)
    {
        m_InputManager->PostTick();
    }

    // 7. 交换缓冲区
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

} // namespace TE
