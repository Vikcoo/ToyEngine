# ToyEngine 单线程立方体渲染 —— UE5 架构学习文档

> 本文档详细讲解 ToyEngine 如何按照 UE5 架构思想，在单线程下实现 3D 旋转立方体渲染。
> 涵盖：UE5 核心概念映射、3D 渲染管线原理、MVP 矩阵、代码架构详解、以及未来双线程改造路线。

---

## 目录

1. [整体架构概览](#1-整体架构概览)
2. [UE5 核心概念映射](#2-ue5-核心概念映射)
3. [模块详解：Scene（游戏侧）](#3-模块详解scene游戏侧)
4. [模块详解：Renderer（渲染侧）](#4-模块详解renderer渲染侧)
5. [核心桥接机制：SyncToScene](#5-核心桥接机制synctoscene)
6. [3D 渲染管线原理](#6-3d-渲染管线原理)
7. [MVP 矩阵详解](#7-mvp-矩阵详解)
8. [深度测试与面剔除](#8-深度测试与面剔除)
9. [单帧数据流完整追踪](#9-单帧数据流完整追踪)
10. [代码结构总览](#10-代码结构总览)
11. [从单线程到双线程的改造路线](#11-从单线程到双线程的改造路线)

---

## 1. 整体架构概览

ToyEngine 采用了 **UE5 的核心架构思想**：**游戏数据与渲染数据分离**。

```
┌─────────────────────────────────────────────────────────┐
│                     Engine (调度中枢)                     │
│                                                          │
│  ┌─────────────┐   SyncToScene   ┌──────────────────┐   │
│  │  TWorld      │ ───────────────▶ │  FScene          │   │
│  │  (游戏世界)  │                  │  (渲染场景)      │   │
│  │              │                  │                  │   │
│  │  TActor      │                  │  SceneProxy      │   │
│  │  └─Component │    桥接          │  └─DrawCommand   │   │
│  └─────────────┘                  └──────────────────┘   │
│         ▲                                  │             │
│         │ Tick                     Render   │             │
│         │                                  ▼             │
│  ┌─────────────┐               ┌──────────────────┐     │
│  │  游戏逻辑    │               │  SceneRenderer   │     │
│  │  (旋转/移动) │               │  (渲染调度)      │     │
│  └─────────────┘               └──────────────────┘     │
│                                          │               │
│                                          ▼               │
│                                 ┌──────────────────┐     │
│                                 │  RHI 抽象层      │     │
│                                 │  (OpenGL/Vulkan) │     │
│                                 └──────────────────┘     │
└─────────────────────────────────────────────────────────┘
```

**为什么要分离？**

| 不分离（简单做法） | 分离（UE5 做法） |
|---|---|
| Engine 直接持有 VBO/IBO/Pipeline | Component 持有逻辑数据，Proxy 持有 GPU 资源 |
| 游戏逻辑和渲染逻辑耦合 | 游戏逻辑只操作 Component，渲染器只看 Proxy |
| 无法多线程 | 天然支持多线程（两个独立的数据集合） |
| 增加物体需要修改 Engine | 增加物体只需创建 Actor + Component |

---

## 2. UE5 核心概念映射

| UE5 概念 | ToyEngine 实现 | 职责 |
|---|---|---|
| `UWorld` | `TWorld` | 游戏世界容器，持有所有 Actor |
| `AActor` | `TActor` | 游戏实体，持有 Component 列表 |
| `UActorComponent` | `TComponent` | 组件基类，提供 Tick() |
| `USceneComponent` | `TSceneComponent` | 带 Transform 的组件 |
| `UPrimitiveComponent` | `TPrimitiveComponent` | 可渲染组件，核心桥接类 |
| `UStaticMeshComponent` | `TMeshComponent` | 网格组件，持有顶点/索引数据 |
| `UCameraComponent` | `TCameraComponent` | 相机组件，构建 FViewInfo |
| `FScene` | `FScene` | 渲染场景，Proxy 容器 |
| `FPrimitiveSceneProxy` | `FPrimitiveSceneProxy` | 渲染侧镜像基类 |
| `FStaticMeshSceneProxy` | `FStaticMeshSceneProxy` | 静态网格 Proxy，持有 GPU 资源 |
| `FSceneView` | `FViewInfo` | 视图信息（View + Projection 矩阵）|
| `FMeshDrawCommand` | `FMeshDrawCommand` | 绘制命令（Pipeline + VBO + IBO + WorldMatrix）|
| `FSceneRenderer` | `SceneRenderer` | 渲染调度器 |

### 核心设计原则

**1. Component-Proxy 配对**
```
TMeshComponent (游戏侧)  ←→  FStaticMeshSceneProxy (渲染侧)
   持有：网格数据描述              持有：VBO/IBO/Shader/Pipeline (GPU 资源)
   操作：SetPosition/Rotate        操作：GetMeshDrawCommand()
```

**2. 数据所有权清晰**
- **游戏逻辑** 只操作 `TWorld` → `TActor` → `TComponent`
- **渲染器** 只访问 `FScene` → `FPrimitiveSceneProxy`
- **Engine** 负责协调两者（SyncToScene）

---

## 3. 模块详解：Scene（游戏侧）

### 3.1 组件继承体系

```
TComponent                    ← 组件基类（Tick, Owner）
  └─ TSceneComponent          ← 增加 Transform（Position/Rotation/Scale）
       ├─ TPrimitiveComponent  ← 增加 CreateSceneProxy() + MarkRenderStateDirty()
       │    └─ TMeshComponent  ← 增加网格数据，override CreateSceneProxy()
       └─ TCameraComponent     ← 增加相机参数，BuildViewInfo()
```

### 3.2 TPrimitiveComponent —— 核心桥接类

这是连接游戏世界和渲染世界的关键类：

```cpp
class TPrimitiveComponent : public TSceneComponent
{
    // 创建渲染侧镜像（子类 override）
    virtual FPrimitiveSceneProxy* CreateSceneProxy(RHIDevice* device);

    // 标记脏：Transform 变化后调用
    void MarkRenderStateDirty();

    // 注册到渲染场景
    void RegisterToScene(FScene* scene, RHIDevice* device);

    FPrimitiveSceneProxy* m_SceneProxy;  // 指向渲染侧镜像
    bool m_RenderStateDirty;             // 脏标记
};
```

**关键流程**：
1. `RegisterToScene()` → 调用 `CreateSceneProxy()` → 创建 Proxy → 加入 `FScene`
2. 游戏逻辑修改 Transform → `MarkRenderStateDirty()` 设置脏标记
3. `SyncToScene()` → 遍历脏 Component → 将 WorldMatrix 同步到 Proxy

### 3.3 TMeshComponent —— 网格组件

```cpp
class TMeshComponent : public TPrimitiveComponent
{
    FStaticMeshData m_MeshData;  // 网格数据描述

    FPrimitiveSceneProxy* CreateSceneProxy(RHIDevice* device) override
    {
        // 创建 FStaticMeshSceneProxy，传入网格数据和 RHIDevice
        return new FStaticMeshSceneProxy(m_MeshData, device);
    }
};
```

### 3.4 TWorld —— 游戏世界

```cpp
class TWorld
{
    vector<unique_ptr<TActor>>      m_Actors;              // Actor 列表
    vector<TPrimitiveComponent*>    m_PrimitiveComponents; // 已注册的可渲染组件

    void Tick(float deltaTime);           // 遍历 Actor Tick
    void SyncToScene(FScene* scene);      // 遍历脏 Component → 同步到 Proxy
};
```

---

## 4. 模块详解：Renderer（渲染侧）

### 4.1 FPrimitiveSceneProxy —— 渲染镜像

```cpp
class FPrimitiveSceneProxy
{
    Matrix4 m_WorldMatrix;  // 世界变换矩阵（由 SyncToScene 同步）

    // 收集绘制命令（子类 override）
    virtual bool GetMeshDrawCommand(FMeshDrawCommand& outCmd) const = 0;
};
```

### 4.2 FStaticMeshSceneProxy —— 静态网格 Proxy

**关键**：GPU 资源在 Proxy 构造时创建，生命周期由 Proxy 管理。

```cpp
class FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
    // RHI 资源（Proxy 拥有所有权）
    unique_ptr<RHIBuffer>   m_VertexBuffer;
    unique_ptr<RHIBuffer>   m_IndexBuffer;
    unique_ptr<RHIShader>   m_VertexShader;
    unique_ptr<RHIShader>   m_FragmentShader;
    unique_ptr<RHIPipeline> m_Pipeline;

    // 构造时通过 RHIDevice 创建 GPU 资源
    FStaticMeshSceneProxy(const FStaticMeshData& data, RHIDevice* device);

    // 填充绘制命令
    bool GetMeshDrawCommand(FMeshDrawCommand& outCmd) const override;
};
```

### 4.3 FScene —— 渲染场景

```cpp
class FScene
{
    vector<FPrimitiveSceneProxy*> m_Primitives;  // Proxy 列表（不拥有内存）
    FViewInfo m_ViewInfo;                         // 当前帧的视图信息

    void AddPrimitive(FPrimitiveSceneProxy* proxy);
    void RemovePrimitive(FPrimitiveSceneProxy* proxy);
};
```

**注意**：FScene 不拥有 Proxy 的内存。Proxy 的生命周期由 TPrimitiveComponent 管理。FScene 只是一个"目录"。

### 4.4 SceneRenderer —— 渲染调度器

这是 UE5 Mesh Draw Pipeline 的极简版本：

```cpp
void SceneRenderer::Render(const FScene* scene, RHIDevice* device, RHICommandBuffer* cmdBuf)
{
    // Step 1: 收集绘制命令（UE5: GatherMeshDrawCommands）
    vector<FMeshDrawCommand> drawCommands;
    for (auto* proxy : scene->GetPrimitives())
    {
        FMeshDrawCommand cmd;
        if (proxy->GetMeshDrawCommand(cmd))
            drawCommands.push_back(cmd);
    }

    // Step 2: 提交到 RHI（UE5: RenderBasePass）
    cmdBuf->Begin();
    cmdBuf->BeginRenderPass(passInfo);

    for (const auto& cmd : drawCommands)
    {
        cmdBuf->BindPipeline(cmd.Pipeline);
        cmdBuf->BindVertexBuffer(cmd.VertexBuffer);
        cmdBuf->BindIndexBuffer(cmd.IndexBuffer);

        Matrix4 mvp = viewProjection * cmd.WorldMatrix;
        cmdBuf->SetUniformMatrix4("u_MVP", mvp.Data());
        cmdBuf->DrawIndexed(cmd.IndexCount);
    }

    cmdBuf->EndRenderPass();
    cmdBuf->End();
}
```

---

## 5. 核心桥接机制：SyncToScene

SyncToScene 是连接游戏世界和渲染世界的桥梁：

```
游戏逻辑修改 Transform
       │
       ▼
MarkRenderStateDirty()  ← 设置脏标记
       │
       ▼ (Tick 结束后)
SyncToScene(FScene*)
       │
       ├─ 遍历所有 PrimitiveComponent
       │    ├─ 检查 IsRenderStateDirty()
       │    └─ 如果脏：
       │         proxy->SetWorldMatrix(comp->GetWorldMatrix())
       │         comp->ClearRenderStateDirty()
       │
       ▼
 Proxy 的 WorldMatrix 已更新，可以渲染了
```

**单线程版本**：SyncToScene 中直接赋值（因为同一线程，没有竞争条件）。

**将来双线程版本**：SyncToScene 改为向渲染线程的命令队列 Enqueue 一个 "UpdateTransform" 命令。

---

## 6. 3D 渲染管线原理

从顶点数据到屏幕像素，经过以下阶段：

```
 CPU 侧                                    GPU 侧
┌──────────┐                        ┌─────────────────────────┐
│ 顶点数据  │ ──── VBO ────────────▶│ 顶点着色器 (Vertex)     │
│ (Position │                        │   input:  aPosition     │
│  + Color) │                        │   uniform: u_MVP        │
│           │                        │   output: gl_Position   │
│ 索引数据  │ ──── IBO ────────────▶│          + vColor       │
│ (三角形)  │                        ├─────────────────────────┤
│           │                        │ 图元装配                 │
│ Pipeline  │ ──── 状态设置 ───────▶│  索引 → 三角形          │
│ (Shader   │                        ├─────────────────────────┤
│  +状态)   │                        │ 光栅化                   │
│           │                        │  三角形 → 像素片段      │
│ Uniform   │                        ├─────────────────────────┤
│ (MVP矩阵)│ ──── glUniform ──────▶│ 片段着色器 (Fragment)   │
│           │                        │   input:  vColor        │
└──────────┘                        │   output: fragColor     │
                                     ├─────────────────────────┤
                                     │ 深度测试 + 面剔除       │
                                     ├─────────────────────────┤
                                     │ 帧缓冲区（屏幕）        │
                                     └─────────────────────────┘
```

### 顶点着色器 (cube.vert)

```glsl
#version 410 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
uniform mat4 u_MVP;
out vec3 vColor;

void main() {
    gl_Position = u_MVP * vec4(aPosition, 1.0);  // 3D → 裁剪空间
    vColor = aColor;                              // 传递颜色给片段着色器
}
```

### 片段着色器 (cube.frag)

```glsl
#version 410 core
in vec3 vColor;
out vec4 fragColor;

void main() {
    fragColor = vec4(vColor, 1.0);  // 直接输出插值后的顶点颜色
}
```

---

## 7. MVP 矩阵详解

MVP = Model × View × Projection，将物体从本地空间变换到屏幕空间。

### 7.1 Model 矩阵（物体空间 → 世界空间）

由 `Transform::ToMatrix()` 生成，顺序：Scale → Rotate → Translate

```
世界空间中：立方体在原点，每帧旋转
Model = Translate(0,0,0) × Rotate(angle, axis) × Scale(1,1,1)
```

### 7.2 View 矩阵（世界空间 → 相机空间）

由 `Matrix4::LookAt(eye, center, up)` 生成：
```
eye    = (2, 2, 3)    ← 相机位置
center = (0, 0, 0)    ← 看向原点
up     = (0, 1, 0)    ← 世界上方
```

LookAt 矩阵将世界坐标变换到以相机为原点、相机朝向为 -Z 的坐标系。

### 7.3 Projection 矩阵（相机空间 → 裁剪空间/NDC）

由 `Matrix4::PerspectiveGL(fov, aspect, near, far)` 生成：
```
fov    = 60°          ← 垂直视场角
aspect = 16/9         ← 宽高比
near   = 0.1          ← 近裁剪面
far    = 100.0        ← 远裁剪面
```

**为什么用 PerspectiveGL 而不是 Perspective？**

项目定义了 `GLM_FORCE_DEPTH_ZERO_TO_ONE`，这使 `glm::perspective()` 生成 Vulkan 风格的 [0,1] 深度范围。但 macOS OpenGL 4.1 不支持 `glClipControl`，深度范围是 [-1,1]。`PerspectiveGL` 内部调用 `glm::perspectiveNO()` 强制使用 [-1,1] 范围。

### 7.4 最终 MVP 计算

在 SceneRenderer 中：
```cpp
Matrix4 mvp = viewProjectionMatrix * worldMatrix;
// = Projection * View * Model
```

注意矩阵乘法顺序：先 Model，再 View，最后 Projection（从右到左）。

---

## 8. 深度测试与面剔除

### 8.1 深度测试（Depth Test）

3D 场景中多个物体可能重叠。深度测试确保近处物体遮挡远处物体：

```
深度缓冲区 (Z-buffer)：
  - 每个像素存储一个深度值
  - 绘制新片段时，比较其深度与缓冲区中的值
  - 如果新片段更近（depth < buffer），写入颜色和深度
  - 如果更远，丢弃
```

在 Pipeline 中启用：
```cpp
pipelineDesc.depthStencil.depthTestEnable = true;
pipelineDesc.depthStencil.depthWriteEnable = true;
pipelineDesc.depthStencil.depthCompareOp = RHICompareOp::Less;
```

### 8.2 面剔除（Face Culling）

立方体是封闭体，看不到的背面不需要渲染：

```
正面 (Front Face)：顶点按逆时针顺序 (CCW)
背面 (Back Face)：顶点按顺时针顺序 (CW)

启用背面剔除：只渲染正面的三角形
  → 减少约 50% 的像素处理量
```

在 Pipeline 中启用：
```cpp
pipelineDesc.rasterization.cullMode = RHICullMode::Back;
pipelineDesc.rasterization.frontFace = RHIFrontFace::CounterClockwise;
```

---

## 9. 单帧数据流完整追踪

```
Engine::Tick(deltaTime)
│
├─ 1. PollEvents()
│     └─ 处理窗口/键盘/鼠标事件
│
├─ 2. 游戏逻辑更新
│     ├─ 找到 CubeActor
│     ├─ actor.GetTransform().RotateWorldY(45° * dt)  ← 旋转立方体
│     ├─ actor.GetTransform().RotateWorldX(30° * dt)
│     └─ meshComp.MarkRenderStateDirty()               ← 标记脏
│
├─ 3. World::Tick(deltaTime)
│     └─ 遍历所有 Actor → 调用 Component.Tick()
│
├─ 4. World::SyncToScene(FScene)                        ← UE5 桥接
│     └─ 遍历脏 PrimitiveComponent:
│          proxy->SetWorldMatrix(comp->GetWorldMatrix())
│          comp->ClearRenderStateDirty()
│
├─ 5. CameraComponent::BuildViewInfo()
│     ├─ ViewMatrix = LookAt(eye, target, up)
│     ├─ ProjectionMatrix = PerspectiveGL(fov, aspect, near, far)
│     └─ ViewProjectionMatrix = Projection * View
│
├─ 6. SceneRenderer::Render(FScene, Device, CmdBuf)
│     ├─ GatherMeshDrawCommands:
│     │     └─ proxy->GetMeshDrawCommand(cmd)
│     │          → cmd = { Pipeline, VBO, IBO, IndexCount=36, WorldMatrix }
│     │
│     └─ SubmitDrawCommands:
│          ├─ CmdBuf->Begin()
│          ├─ CmdBuf->BeginRenderPass(清屏+视口)
│          ├─ CmdBuf->BindPipeline(pipeline)
│          ├─ CmdBuf->BindVertexBuffer(vbo)
│          ├─ CmdBuf->BindIndexBuffer(ibo)
│          ├─ CmdBuf->SetUniformMatrix4("u_MVP", mvp)
│          ├─ CmdBuf->DrawIndexed(36)
│          ├─ CmdBuf->EndRenderPass()
│          └─ CmdBuf->End()
│
└─ 7. Window::SwapBuffers()
      └─ 前后缓冲区交换 → 屏幕显示
```

---

## 10. 代码结构总览

```
Source/Runtime/
├── Core/                    基础设施
│   ├── Public/Math/
│   │   ├── MathTypes.h      Vector2/3/4, Matrix3/4, Quat（+ PerspectiveGL）
│   │   ├── Transform.h      Transform 类（Position+Rotation+Scale → ToMatrix()）
│   │   └── MathUtils.h      Math::DegToRad, Lerp, Clamp 等工具函数
│   └── Public/Log/          日志系统（TE_LOG_XXX）
│
├── Platform/                窗口抽象
│   └── GLFWWindow           OpenGL 4.1 + GLFW 窗口
│
├── RHI/                     GPU 抽象接口
│   └── Public/
│       ├── RHIDevice.h           创建 Buffer/Shader/Pipeline/CommandBuffer
│       ├── RHICommandBuffer.h    命令录制（Draw/Bind/SetUniform）
│       ├── RHIBuffer.h           顶点/索引缓冲区
│       ├── RHIShader.h           着色器模块
│       ├── RHIPipeline.h         图形管线状态
│       └── RHITypes.h            枚举和描述符结构体
│
├── OpenGLRHI/               OpenGL 后端
│   └── Private/
│       ├── OpenGLDevice.cpp        Device 实现
│       ├── OpenGLCommandBuffer.cpp CmdBuf 实现（+ SetUniformMatrix4）
│       ├── OpenGLPipeline.cpp      Pipeline 实现
│       ├── OpenGLBuffer.cpp        Buffer 实现
│       └── OpenGLShader.cpp        Shader 实现
│
├── Scene/                   ★ 游戏侧数据（UE5 World/Actor/Component）
│   ├── Public/
│   │   ├── Component.h           TComponent 基类
│   │   ├── SceneComponent.h      TSceneComponent（+ Transform）
│   │   ├── PrimitiveComponent.h  TPrimitiveComponent（核心桥接类）
│   │   ├── MeshComponent.h       TMeshComponent（网格数据）
│   │   ├── CameraComponent.h     TCameraComponent（FViewInfo）
│   │   ├── Actor.h               TActor（组件列表）
│   │   └── World.h               TWorld（Actor 列表 + SyncToScene）
│   └── Private/                  实现文件
│
├── Renderer/                ★ 渲染侧数据 + 渲染器
│   ├── Public/
│   │   ├── FPrimitiveSceneProxy.h    渲染镜像基类
│   │   ├── FStaticMeshSceneProxy.h   静态网格 Proxy（持有 GPU 资源）
│   │   ├── FScene.h                  渲染场景（Proxy 容器）
│   │   ├── FViewInfo.h               视图信息
│   │   ├── FMeshDrawCommand.h        绘制命令数据
│   │   └── SceneRenderer.h           渲染调度器
│   └── Private/                      实现文件
│
└── Engine/                  引擎主循环
    ├── Public/Engine.h      持有 World + FScene + SceneRenderer
    └── Private/Engine.cpp   Init/Run/Tick/Shutdown + UE5 数据流

Content/Shaders/OpenGL/
├── cube.vert               立方体顶点着色器（u_MVP 变换）
├── cube.frag               立方体片段着色器（顶点颜色）
├── triangle.vert           （旧）三角形着色器
└── triangle.frag           （旧）三角形着色器
```

---

## 11. 从单线程到双线程的改造路线

当前的单线程架构已经按 UE5 思想设计，保留了所有关键的接口和数据分离。将来改造为双线程只需在几个关键点做修改：

### 改造点对照表

| 单线程做法 | 双线程改造 | 影响范围 |
|---|---|---|
| `SyncToScene()` 中直接赋值 `proxy->SetWorldMatrix()` | 改为 `RenderCommandQueue.Enqueue(UpdateTransformCmd)` | World.cpp |
| `SceneRenderer::Render()` 在主线程直接调用 | 改为在渲染线程调用 | Engine.cpp |
| `FScene` 在主线程直接访问 | 渲染线程独占访问，主线程通过命令队列间接操作 | FScene 加锁或双缓冲 |
| 没有同步点 | 新增 `RenderCommandFence` 等待渲染线程完成 | Engine.cpp |

### 需要新增的组件

```
Source/Runtime/
├── RenderThread/              ★ 新增
│   ├── RenderThread.h         渲染线程管理
│   ├── RenderCommandQueue.h   命令队列（lock-free queue）
│   └── RenderCommandFence.h   同步栅栏
```

### 改造后的数据流

```
Game Thread                          Render Thread
─────────────                        ─────────────
World::Tick()                        (等待命令...)
    │
SyncToScene()
    │ Enqueue(UpdateTransform) ─────▶ 从队列取命令
    │ Enqueue(UpdateViewInfo)  ─────▶ proxy->SetWorldMatrix()
    │                                 scene->SetViewInfo()
    │
Signal(Fence) ─────────────────────▶ SceneRenderer::Render()
    │                                     │
(继续下一帧逻辑)                          ├─ GatherDrawCommands
    │                                     ├─ SubmitToRHI
    │                                     └─ SwapBuffers
    │
Wait(Fence) ◀──────────────────────── Signal(完成)
```

### 不需要修改的部分

- ✅ Component/Actor/World 的接口（只改 SyncToScene 内部实现）
- ✅ FPrimitiveSceneProxy 的接口（SetWorldMatrix/GetMeshDrawCommand）
- ✅ SceneRenderer 的接口（Render 方法签名不变）
- ✅ RHI 接口（不变）
- ✅ Shader（不变）

**这就是 UE5 架构分离的威力：游戏侧和渲染侧的接口不变，只需修改桥接层的实现。**

---

## 附录：立方体顶点数据

24 个顶点（每面 4 个独立顶点，不共享，每面不同颜色）：

| 面 | 方向 | 颜色 | 顶点索引 |
|---|---|---|---|
| 前面 | Z+ | 红色 (0.9, 0.2, 0.2) | 0-3 |
| 后面 | Z- | 绿色 (0.2, 0.9, 0.2) | 4-7 |
| 右面 | X+ | 蓝色 (0.2, 0.2, 0.9) | 8-11 |
| 左面 | X- | 黄色 (0.9, 0.9, 0.2) | 12-15 |
| 顶面 | Y+ | 青色 (0.2, 0.9, 0.9) | 16-19 |
| 底面 | Y- | 品红 (0.9, 0.2, 0.9) | 20-23 |

36 个索引（每面 2 个三角形，CCW 顺序）：
```
前面: 0,1,2, 2,3,0    后面: 4,5,6, 6,7,4
右面: 8,9,10, 10,11,8  左面: 12,13,14, 14,15,12
顶面: 16,17,18, 18,19,16  底面: 20,21,22, 22,23,20
```
