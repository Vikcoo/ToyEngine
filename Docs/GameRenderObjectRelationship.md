# 游戏侧 vs 渲染侧对象关系详解

> 本文档结合 ToyEngine 自身代码，详细解释游戏侧和渲染侧每个对象的职责、区别和联系。

---

## 为什么要分成两侧？

一句话：**逻辑和渲染是两件完全不同的事，用同一个对象来管会变得非常混乱。**

想象一个角色在游戏中移动：
- **游戏逻辑**关心：角色在哪里（Position）、朝向哪里（Rotation）、要不要碰撞、血量多少
- **渲染**关心：这个角色的三角形数据在 GPU 哪块内存、用什么 Shader 画、用什么材质

如果把这两部分塞在同一个类里，类会变得巨大无比，而且：
- 改游戏逻辑可能意外影响渲染
- 改渲染可能意外影响游戏逻辑
- 将来想把渲染放到单独线程时，到处是数据竞争

所以 UE5（和你的 ToyEngine）把它们分开成**两个平行的世界**。

---

## 一、所有对象一览表

### 游戏侧对象（T 前缀 / Scene 模块）

| 对象 | UE5 对应 | 职责 | 持有的数据 |
|------|---------|------|-----------|
| `TComponent` | `UActorComponent` | 组件基类 | Owner 指针、名称 |
| `TSceneComponent` | `USceneComponent` | 带 Transform 的组件 | Position / Rotation / Scale |
| `TPrimitiveComponent` | `UPrimitiveComponent` | **可渲染组件（桥接类）** | SceneProxy 指针、脏标记 |
| `TMeshComponent` | `UStaticMeshComponent` | 网格组件 | `shared_ptr<TStaticMesh>` 资产引用 |
| `TCameraComponent` | `UCameraComponent` | 相机组件 | FOV / AspectRatio / Near / Far |
| `TActor` | `AActor` | 实体（组件容器） | 组件列表、RootComponent |
| `TWorld` | `UWorld` | 游戏世界 | Actor 列表、PrimitiveComponent 列表 |

### 渲染侧对象（F 前缀 / Renderer 模块）

| 对象 | UE5 对应 | 职责 | 持有的数据 |
|------|---------|------|-----------|
| `FPrimitiveSceneProxy` | `FPrimitiveSceneProxy` | 渲染镜像基类 | WorldMatrix |
| `FStaticMeshSceneProxy` | `FStaticMeshSceneProxy` | 静态网格渲染镜像 | VBO / IBO / Shader / Pipeline（GPU 资源） |
| `FScene` | `FScene` | 渲染场景容器 | Proxy 列表 + ViewInfo |
| `FViewInfo` | `FSceneView` / `FViewInfo` | 视图信息 | View / Projection / VP 矩阵、视口尺寸 |
| `FMeshDrawCommand` | `FMeshDrawCommand` | 绘制命令 | Pipeline + VBO + IBO + IndexCount + WorldMatrix |
| `SceneRenderer` | `FSceneRenderer` | 渲染调度器 | 无状态，每帧收集并提交绘制命令 |

---

## 二、逐对象深度解析

### 2.1 TComponent → 万物基类

```
TComponent
├── 持有：Owner(TActor*)、Name
├── 方法：Tick(deltaTime)
└── 意义：所有组件的公共接口
```

**它不属于游戏侧也不属于渲染侧**，它只是一个"我是某个 Actor 的组件"的契约。单纯的 `TComponent` 没有位置、没有可视化能力，什么都画不了。

类比现实：`TComponent` 就像一个"器官"的概念——心脏是器官，眼睛也是器官，但"器官"本身不做任何具体事情。

---

### 2.2 TSceneComponent → 有了位置

```
TSceneComponent : TComponent
├── 新增持有：Transform (Position + Rotation + Scale)
├── 新增方法：GetWorldMatrix() → 返回 4x4 变换矩阵
└── 意义：在 3D 空间中有了位置
```

**纯游戏侧对象**。它只知道"我在哪里"，不知道"我长什么样"。

类比现实：就像一个**空气中的坐标点**——你知道它的位置，但看不到任何东西。

---

### 2.3 TPrimitiveComponent → ⭐ 桥接类（最关键！）

```
TPrimitiveComponent : TSceneComponent
├── 新增持有：
│   ├── m_SceneProxy (FPrimitiveSceneProxy*) — 指向渲染侧镜像
│   └── m_RenderStateDirty (bool) — 脏标记
├── 核心方法：
│   ├── CreateSceneProxy(device) → 创建渲染侧镜像（子类 override）
│   ├── RegisterToScene(scene, device) → 创建 Proxy 并加入 FScene
│   ├── UnregisterFromScene(scene) → 从 FScene 移除并销毁 Proxy
│   └── MarkRenderStateDirty() → 通知渲染侧数据已变化
└── 意义：游戏侧和渲染侧的连接点
```

**这是整个架构最关键的类！** 它是游戏世界和渲染世界之间的**桥梁**：

- 它**属于游戏侧**（继承自 TSceneComponent，有 Transform）
- 但它**知道渲染侧的存在**（持有 SceneProxy 指针）
- 它的核心职责就是：**把游戏侧的变化通知到渲染侧**

脏标记机制：
```
游戏逻辑修改 Transform
    → MarkRenderStateDirty()    // 标记："我变了"
    → ...
World::SyncToScene() 时
    → 检查 IsRenderStateDirty()
    → 如果脏：proxy->SetWorldMatrix(comp->GetWorldMatrix())  // 同步到渲染侧
    → ClearRenderStateDirty()   // 清除脏标记
```

类比现实：`TPrimitiveComponent` 就像一个**翻译官**——游戏侧说"我移到了这个位置"，翻译官就把这个信息传达给渲染侧。

---

### 2.4 TMeshComponent → 游戏侧的"这个物体长什么样"

```
TMeshComponent : TPrimitiveComponent
├── 新增持有：shared_ptr<TStaticMesh> — 网格资产引用
├── 核心方法：
│   ├── SetStaticMesh(mesh) → 设置要显示的网格
│   └── CreateSceneProxy(device) → 创建 FStaticMeshSceneProxy
└── 意义：告诉引擎"这个组件要显示这个模型"
```

**纯游戏侧对象**。它只持有"要显示什么模型"的**引用**（`shared_ptr<TStaticMesh>`），不持有任何 GPU 资源。

`CreateSceneProxy()` 是它最重要的方法——它把资产数据交给渲染侧，让渲染侧去创建 GPU 资源：

```cpp
FPrimitiveSceneProxy* TMeshComponent::CreateSceneProxy(RHIDevice* device)
{
    // 把 TStaticMesh（CPU 数据）传给 FStaticMeshSceneProxy
    // Proxy 在构造函数中把数据上传到 GPU
    return new FStaticMeshSceneProxy(m_StaticMesh.get(), device);
}
```

类比现实：`TMeshComponent` 就像一个**设计图纸**——图纸上画了"这个物体是个立方体"，但图纸本身不是实物。`CreateSceneProxy()` 就是"把图纸交给工厂去生产"。

---

### 2.5 FPrimitiveSceneProxy → 渲染侧的影子

```
FPrimitiveSceneProxy（抽象基类）
├── 持有：m_WorldMatrix — 世界变换矩阵
├── 方法：
│   ├── SetWorldMatrix() / GetWorldMatrix() — 被 SyncToScene 调用
│   └── GetMeshDrawCommands(outCommands) → 纯虚方法，子类填充绘制命令
└── 意义：渲染侧的可渲染对象基类
```

**纯渲染侧对象**。它不知道 `TActor` 是什么，不知道 `TWorld` 是什么。它只知道：
- 我在世界空间的什么位置（WorldMatrix）
- 怎么把自己画出来（GetMeshDrawCommands）

类比现实：`FPrimitiveSceneProxy` 是游戏对象在渲染世界中的**影子**——它跟着游戏对象移动，但它自己独立存在，有自己的数据（GPU 资源）。

---

### 2.6 FStaticMeshSceneProxy → 渲染侧的"真正负责画画的人"

```
FStaticMeshSceneProxy : FPrimitiveSceneProxy
├── 持有：
│   ├── m_SectionGPUData[] — 每个 Section 的 VBO + IBO（GPU 资源！）
│   ├── m_VertexShader / m_FragmentShader — Shader
│   └── m_Pipeline — 渲染管线状态
├── 构造函数：从 TStaticMesh 读取顶点/索引数据 → 上传到 GPU
├── 核心方法：
│   └── GetMeshDrawCommands() → 为每个 Section 生成一条 FMeshDrawCommand
└── 意义：静态网格的渲染实现
```

**纯渲染侧对象，持有真正的 GPU 资源**。这是整个渲染的核心工作者。

构造时做的事（只在注册时执行一次）：
```
TStaticMesh（CPU 数据）
    │
    ├─ Section[0].Vertices → device->CreateBuffer() → VBO[0]（GPU）
    ├─ Section[0].Indices  → device->CreateBuffer() → IBO[0]（GPU）
    ├─ Section[1].Vertices → device->CreateBuffer() → VBO[1]（GPU）
    ├─ Section[1].Indices  → device->CreateBuffer() → IBO[1]（GPU）
    └─ ...
    
    + CreateShader() → Vertex/Fragment Shader
    + CreatePipeline() → 渲染管线
```

每帧做的事（被 SceneRenderer 调用）：
```
GetMeshDrawCommands() → 为每个 Section 返回一条 FMeshDrawCommand：
    cmd.Pipeline     = m_Pipeline
    cmd.VertexBuffer = m_SectionGPUData[i].VBO
    cmd.IndexBuffer  = m_SectionGPUData[i].IBO
    cmd.IndexCount   = m_SectionGPUData[i].IndexCount
    cmd.WorldMatrix  = m_WorldMatrix   // 从 SyncToScene 同步来的
```

类比现实：`FStaticMeshSceneProxy` 就像**工厂里的一台机器**——它持有模具（GPU 资源），知道怎么把产品（三角形）渲染到屏幕上。

---

### 2.7 FScene → 渲染世界的容器

```
FScene
├── 持有：
│   ├── m_Primitives: vector<FPrimitiveSceneProxy*> — 所有渲染对象
│   └── m_ViewInfo: FViewInfo — 当前帧的相机信息
├── 方法：AddPrimitive() / RemovePrimitive()
└── 意义：SceneRenderer 的唯一数据来源
```

**纯渲染侧对象**。对应游戏侧的 `TWorld`，但它完全不知道 `TWorld` 的存在。

关键特性：**FScene 不拥有 Proxy 的内存**（只持有 raw pointer）。Proxy 的生命周期由 `TPrimitiveComponent` 管理。

类比现实：`FScene` 就像一个**舞台**——演员（Proxy）在舞台上表演，但舞台不管演员的生死（生命周期由经纪公司 TPrimitiveComponent 管）。

---

### 2.8 TWorld → 游戏世界的容器

```
TWorld
├── 持有：
│   ├── m_Actors: vector<unique_ptr<TActor>> — 所有游戏实体
│   ├── m_PrimitiveComponents: vector<TPrimitiveComponent*> — 所有可渲染组件
│   ├── m_Scene: FScene* — 渲染场景引用
│   └── m_RHIDevice: RHIDevice* — RHI 设备引用
├── 方法：
│   ├── Tick(deltaTime) → 遍历 Actor 更新逻辑
│   └── SyncToScene(FScene*) → 遍历脏组件同步到 Proxy
└── 意义：游戏逻辑的总管理者
```

**游戏侧对象，但知道 FScene 的存在**（为了 SyncToScene）。

---

### 2.9 SceneRenderer → 渲染的大脑

```
SceneRenderer
├── 无状态（没有成员变量需要跨帧保持）
├── 核心方法：Render(scene, device, cmdBuf)
│   ├── GatherMeshDrawCommands() → 遍历 FScene 所有 Proxy，收集绘制命令
│   └── SubmitDrawCommands()     → 逐命令：BindPipeline → BindVBO → BindIBO → SetUniform(MVP) → DrawIndexed
└── 意义：决定"怎么画"、"画什么顺序"
```

**纯渲染侧对象**。它只和 FScene、RHI 打交道，完全不知道游戏侧的任何东西。

---

## 三、对象关系全景图

```
                        游戏侧（Scene 模块）                                     渲染侧（Renderer 模块）
                    ┌─────────────────────────┐                           ┌───────────────────────────┐
                    │         TWorld           │                           │          FScene            │
                    │  ├─ TActor (MeshActor)   │                           │  ├─ FStaticMeshSceneProxy* │
                    │  │   └─ TMeshComponent ──┼──── CreateSceneProxy() ──→│  │   ├─ VBO (GPU)          │
                    │  │       ├─ TStaticMesh  │                           │  │   ├─ IBO (GPU)          │
                    │  │       │   (CPU 数据)   │                           │  │   ├─ Shader             │
                    │  │       └─ Transform    │                           │  │   ├─ Pipeline           │
                    │  │                       │                           │  │   └─ WorldMatrix        │
                    │  └─ TActor (CameraActor) │                           │  │                         │
                    │      └─ TCameraComponent ┼── BuildViewInfo() ──────→│  └─ FViewInfo              │
                    │          ├─ FOV          │                           │      ├─ ViewMatrix         │
                    │          ├─ Near/Far     │                           │      └─ ProjectionMatrix   │
                    │          └─ Transform    │                           └───────────┬───────────────┘
                    └──────────┬──────────────┘                                       │
                               │                                                      │
                               │  SyncToScene()                                       │
                               │  （每帧：脏 Component → Proxy.SetWorldMatrix()）      │
                               └──────────────────────────────────────────────────────→│
                                                                                      │
                                                                              SceneRenderer::Render()
                                                                                      │
                                                                    ┌─────────────────┴──────────────────┐
                                                                    │  1. GatherMeshDrawCommands()        │
                                                                    │     遍历 Proxy → FMeshDrawCommand   │
                                                                    │  2. SubmitDrawCommands()            │
                                                                    │     BindPipeline → BindVBO → Draw   │
                                                                    └────────────────────────────────────┘
                                                                                      │
                                                                                      ▼
                                                                               RHI (OpenGL)
                                                                               屏幕输出
```

---

## 四、关键流程详解

### 4.1 注册流程：游戏对象如何出现在渲染世界中

```
① Engine::BuildScene()
    ├─ 创建 TActor + TMeshComponent
    ├─ meshComp->SetStaticMesh(loadedMesh)     // 设置资产引用
    └─ world->AddActor(meshActor)
    
② TWorld::AddActor()
    ├─ 遍历 Actor 所有组件
    ├─ 找到 TPrimitiveComponent 子类
    └─ primComp->RegisterToScene(scene, device)
    
③ TPrimitiveComponent::RegisterToScene()
    ├─ m_SceneProxy = CreateSceneProxy(device)   // 调用子类 override
    │       │
    │       ▼ TMeshComponent::CreateSceneProxy()
    │           └─ new FStaticMeshSceneProxy(staticMesh, device)
    │                   │
    │                   ├─ 读取 TStaticMesh 的 Section 数据（CPU）
    │                   ├─ device->CreateBuffer() → VBO/IBO（GPU）
    │                   └─ device->CreateShader/Pipeline()
    │
    ├─ proxy->SetWorldMatrix(GetWorldMatrix())   // 初始化位置
    └─ scene->AddPrimitive(proxy)                // 加入渲染场景
```

**结果：** 游戏侧的 `TMeshComponent` 通过 `CreateSceneProxy()` 在渲染侧创造了自己的影子 `FStaticMeshSceneProxy`，并把影子放入了渲染舞台 `FScene`。

---

### 4.2 每帧同步：游戏侧的变化如何传递到渲染侧

```
Engine::Tick(deltaTime)
│
├─ ① 游戏逻辑修改数据
│     actor->GetTransform().RotateWorldY(angle)
│     primComp->MarkRenderStateDirty()     ← 标记："我变了！"
│
├─ ② World::Tick(deltaTime)
│     遍历所有 Actor → 各 Component 的 Tick()
│
├─ ③ World::SyncToScene(scene)            ← 桥接！
│     for comp in PrimitiveComponents:
│         if comp->IsRenderStateDirty():
│             proxy = comp->GetSceneProxy()
│             proxy->SetWorldMatrix(comp->GetWorldMatrix())  ← 把新位置告诉渲染侧
│             comp->ClearRenderStateDirty()
│
├─ ④ CameraComponent::BuildViewInfo()
│     构建 View + Projection 矩阵 → FViewInfo
│     scene->SetViewInfo(viewInfo)
│
├─ ⑤ SceneRenderer::Render(scene, device, cmdBuf)
│     GatherMeshDrawCommands()  ← 从 Proxy 收集绘制命令
│     SubmitDrawCommands()      ← 通过 RHI 执行绘制
│
└─ ⑥ Window::SwapBuffers()     ← 显示到屏幕
```

---

### 4.3 销毁流程：游戏对象如何从渲染世界消失

```
TPrimitiveComponent 析构
    ├─ delete m_SceneProxy        // 销毁 GPU 资源
    └─ m_SceneProxy = nullptr

或者显式调用：
    TPrimitiveComponent::UnregisterFromScene(scene)
        ├─ scene->RemovePrimitive(proxy)  // 从 FScene 移除
        ├─ delete proxy                    // 销毁 GPU 资源
        └─ m_SceneProxy = nullptr
```

---

## 五、对象所有权总结

理解所有权可以避免内存泄漏和野指针：

```
Engine
 ├─ owns → TWorld (unique_ptr)
 │          ├─ owns → TActor[] (unique_ptr)
 │          │          └─ owns → TComponent[] (unique_ptr)
 │          │                     └─ TPrimitiveComponent
 │          │                          └─ owns → FPrimitiveSceneProxy (raw new/delete)
 │          │                                     └─ owns → VBO/IBO/Shader/Pipeline (unique_ptr)
 │          └─ refs → TPrimitiveComponent[] (raw pointer, 不拥有)
 │
 ├─ owns → FScene (unique_ptr)
 │          └─ refs → FPrimitiveSceneProxy[] (raw pointer, 不拥有)
 │
 ├─ owns → SceneRenderer (unique_ptr)
 │
 ├─ owns → RHIDevice (unique_ptr)
 ├─ owns → RHICommandBuffer (unique_ptr)
 └─ owns → TStaticMesh (shared_ptr, 可被多个 TMeshComponent 共享)
```

关键：
- `unique_ptr` = 独占所有权，销毁时自动释放
- `shared_ptr` = 共享所有权（TStaticMesh 资产可被多个组件引用）
- `raw pointer` = 不拥有，只是引用（FScene 不负责销毁 Proxy）

---

## 六、常见疑问

### Q: 为什么不直接在 TMeshComponent 里画？

如果在 `TMeshComponent` 里直接调用 OpenGL：
1. 游戏逻辑和渲染代码混在一起，改一个可能坏另一个
2. 将来想换渲染后端（Vulkan/Metal），得改所有 Component
3. 将来想多线程渲染，到处是数据竞争
4. 无法排序渲染顺序（先画不透明、再画半透明）

### Q: FScene 和 TWorld 为什么不合并？

因为渲染线程只需要渲染数据（Proxy + ViewInfo），不需要游戏数据（Actor + Component + 血量 + AI状态...）。分开后：
- 渲染线程只访问 FScene，无需加锁
- 游戏线程只修改 TWorld，无需担心渲染
- SyncToScene() 是唯一的数据交换点，好控制

### Q: 为什么 Proxy 的内存由 TPrimitiveComponent 管理而不是 FScene？

因为 Component 知道 Proxy 什么时候该创建（`RegisterToScene`）和销毁（`UnregisterFromScene` / 析构）。FScene 只是一个容器，不应该管生命周期。

### Q: SyncToScene 为什么只同步 WorldMatrix？

目前只有 Transform 会变化。将来如果材质也能动态改变，也会通过类似的脏标记机制同步材质数据到 Proxy。

### Q: TStaticMesh 是什么侧的？

**两边都不算，它是资产（Asset）**。TStaticMesh 持有的是 CPU 侧的原始顶点/索引数据，不属于游戏逻辑，也不属于渲染。它是一个**中间数据格式**：
- 游戏侧（TMeshComponent）通过 `shared_ptr` 引用它
- 渲染侧（FStaticMeshSceneProxy）在构造时读取它的数据上传到 GPU

---

更新日期：2026-03-25
