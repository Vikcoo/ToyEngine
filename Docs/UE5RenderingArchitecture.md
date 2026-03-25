# UE5 渲染架构分析 & ToyEngine 对标指南

> 本文档详细分析 UE5 的渲染架构设计，并对照 ToyEngine 当前实现，给出进一步对齐的改进方向。

---

## 1. 核心分层：三大子系统

UE5 的渲染系统严格分为三层，各自职责清晰，通过明确的数据流向连接：

```
┌─────────────────────────────────────────────────────────┐
│  游戏线程 (Game Thread)                                   │
│  UWorld → AActor → UPrimitiveComponent                   │
│  持有逻辑数据（Transform、材质引用、LOD 策略等）            │
└────────────────────────┬────────────────────────────────┘
                         │ CreateSceneProxy() + SyncToRenderThread
                         ▼
┌─────────────────────────────────────────────────────────┐
│  渲染线程 (Render Thread)                                 │
│  FScene → FPrimitiveSceneProxy → FMeshDrawCommand        │
│  FSceneRenderer (FDeferredShadingSceneRenderer)          │
│  FViewInfo / FSceneView                                   │
│  持有渲染侧数据（GPU 资源引用、绘制命令、Shader 绑定等）   │
└────────────────────────┬────────────────────────────────┘
                         │ RHI Command (FRHICommandList)
                         ▼
┌─────────────────────────────────────────────────────────┐
│  RHI 线程 (RHI Thread)                                    │
│  FDynamicRHI → FD3D12DynamicRHI / FVulkanDynamicRHI      │
│  实际提交给 GPU 的 API 调用                                │
└─────────────────────────────────────────────────────────┘
```

### ToyEngine 当前对应关系

| UE5 层 | ToyEngine 实现 |
|--------|---------------|
| 游戏线程 | `TWorld` / `TActor` / `TPrimitiveComponent` |
| 渲染线程 | `FScene` / `FPrimitiveSceneProxy` / `SceneRenderer` |
| RHI 线程 | `RHIDevice` / `RHICommandBuffer` / `OpenGLRHI` |

> **注**：ToyEngine 当前为单线程版本，三层在同一线程中顺序执行。

---

## 2. FSceneRenderer 设计（核心渲染调度器）

### 2.1 生命周期：每帧临时创建

UE5 中 `FSceneRenderer` 是**每帧临时创建**的对象（不是常驻的）：

```cpp
// UE5 简化伪代码
void FRendererModule::BeginRenderingViewFamily(FSceneViewFamily* ViewFamily)
{
    // 1. 根据渲染路径创建对应的 SceneRenderer
    FSceneRenderer* SceneRenderer = nullptr;
    if (UseDeferredShading)
        SceneRenderer = new FDeferredShadingSceneRenderer(Scene, ViewFamily);
    else
        SceneRenderer = new FMobileSceneRenderer(Scene, ViewFamily);
    
    // 2. 渲染
    SceneRenderer->Render();
    
    // 3. 帧结束后销毁
    delete SceneRenderer;
}
```

### 2.2 类层级结构

```
FSceneRenderer (抽象基类)
├── FDeferredShadingSceneRenderer  (PC/Console 延迟渲染)
└── FMobileSceneRenderer           (移动端 Forward 渲染)
```

> ToyEngine 的 `SceneRenderer` 对应 `FSceneRenderer`。后续可通过子类区分渲染路径（Deferred vs Forward）。

### 2.3 Render() 核心流程

```cpp
void FDeferredShadingSceneRenderer::Render()
{
    // ====== Phase 1: 可见性裁剪 ======
    InitViews();           // 视锥体裁剪 + 遮挡剔除
    ComputeVisibility();   // 确定每个 View 可见的 Primitive 列表
    
    // ====== Phase 2: 收集绘制命令 (Mesh Draw Pipeline) ======
    // 遍历可见 Proxy → 获取 FMeshBatch → 生成 FMeshDrawCommand
    SetupMeshPass();       // 为每个 Pass 收集 FMeshDrawCommand
    
    // ====== Phase 3: 多 Pass 渲染 ======
    RenderPrePass();       // PrePass (Depth Only) - 提前深度写入
    RenderBasePass();      // BasePass (GBuffer) - 输出 Albedo/Normal/Metallic/Roughness
    RenderLights();        // Lighting - 读取 GBuffer，逐光源累加光照
    RenderTranslucency();  // Translucency - 半透明物体（Forward 渲染）
    RenderPostProcess();   // PostProcess - Bloom/DOF/ToneMapping/AA 等
    
    // ====== Phase 4: 呈现 ======
    // Present to screen
}
```

> ToyEngine 当前的 `SceneRenderer::Render()` 是最简化的 Forward 单 Pass 版本。

---

## 3. Mesh Draw Pipeline（核心数据管线）

这是 UE5 5.x 最核心的渲染数据管线，数据从游戏侧 Component 最终转变为 GPU 绘制命令的完整过程：

```
UPrimitiveComponent::CreateSceneProxy()
         │
         ▼
FPrimitiveSceneProxy
         │ GetDynamicMeshElements() 或 GetStaticMeshElements()
         ▼
FMeshBatch  (描述一个网格批次：Section + Material + LOD)
         │ FMeshPassProcessor::AddMeshBatch()
         ▼
FMeshDrawCommand  (最终绘制命令：Pipeline + ShaderBindings + VBO/IBO + DrawArgs)
         │ 缓存到 FScene 或逐帧生成
         ▼
SubmitMeshDrawCommands()  →  RHI CommandList
```

### ToyEngine 对应关系

| UE5 概念 | ToyEngine 实现 | 状态 |
|---------|---------------|------|
| `FPrimitiveSceneProxy::GetDynamicMeshElements()` | `FPrimitiveSceneProxy::GetMeshDrawCommands()` | ✅ 已对齐 |
| `FMeshBatch` | 跳过（直接到 DrawCommand） | ⬜ 未实现 |
| `FMeshPassProcessor` | 跳过 | ⬜ 未实现 |
| `FMeshDrawCommand` | `FMeshDrawCommand` | ✅ 已对齐 |
| `FMeshDrawCommand` 缓存 | 每帧重新收集 | ⬜ 未实现 |

### 3.1 FMeshBatch 详解

`FMeshBatch` 是 Proxy 和 DrawCommand 之间的中间层，包含：

```cpp
struct FMeshBatch
{
    const FVertexFactory* VertexFactory;     // 顶点工厂
    const FMaterialRenderProxy* MaterialProxy; // 材质代理
    FMeshBatchElement Elements[...];          // 批次元素（Section/LOD）
    uint8 LODIndex;
    uint8 SegmentIndex;
    // ...
};
```

### 3.2 FMeshPassProcessor 详解

每种 Pass 有独立的 Processor，决定如何将 MeshBatch 转换为 DrawCommand：

```cpp
class FMeshPassProcessor {
public:
    virtual void AddMeshBatch(
        const FMeshBatch& batch,
        uint64 batchElementMask,
        const FPrimitiveSceneProxy* proxy
    ) = 0;
};

// 具体实现
class FBasePassMeshProcessor : public FMeshPassProcessor { ... };
class FDepthPassMeshProcessor : public FMeshPassProcessor { ... };
class FShadowDepthPassMeshProcessor : public FMeshPassProcessor { ... };
class FTranslucencyPassMeshProcessor : public FMeshPassProcessor { ... };
```

---

## 4. FScene 的角色

```cpp
// UE5 FScene 简化版
class FScene : public FSceneInterface
{
    // Primitive 管理
    TArray<FPrimitiveSceneInfo*> Primitives;
    TArray<FPrimitiveSceneProxy*> PrimitiveProxies;
    
    // 光源管理
    TArray<FLightSceneInfo*> Lights;
    
    // 加速结构
    FSceneOctree PrimitiveOctree;  // 空间八叉树（用于裁剪）
    
    // 缓存的 MeshDrawCommand（按 Pass 类型）
    FCachedPassMeshDrawList CachedDrawLists[EMeshPass::Num];
};
```

> ToyEngine 的 `FScene` 已有核心骨架（Primitives + ViewInfo），后续可扩展光源和缓存。

---

## 5. Engine 和 Renderer 的关系

在 UE5 中，**Engine 完全不直接调用 RHI**。数据流：

```
Engine (UGameEngine)
  → 拥有 UWorld (游戏侧)
  → UWorld 通过 FScene 接口注册 Proxy
  → FRendererModule 负责调用 FSceneRenderer::Render()
  → FSceneRenderer 通过 FRHICommandList 调用 RHI
```

### Engine 的职责（仅限）
1. 初始化子系统（创建 RHI Device、窗口等）
2. 驱动 Tick 循环
3. 触发渲染（调用 Renderer 模块的入口）

### Renderer 的职责
1. 从 FScene 获取数据
2. 执行裁剪和可见性
3. 组织多 Pass 渲染
4. 通过 RHI 提交绘制命令

---

## 6. FSceneViewFamily 与多 View 支持

UE5 中一帧可以渲染多个 View（分屏、编辑器多视口等）：

```cpp
struct FSceneViewFamily {
    TArray<FViewInfo> Views;  // 一组视图
    FScene* Scene;
    float DeltaTime;
    // 渲染设置（AA 方式、后处理开关等）
};
```

---

## 7. ToyEngine 当前状态总览

### ✅ 已完成

| 设计原则 | 说明 |
|---------|------|
| Game/Render 数据分离 | Component 不直接接触 GPU 资源 |
| Proxy 模式 | 渲染侧通过 SceneProxy 镜像游戏侧数据 |
| FMeshDrawCommand | 已实现绘制命令收集 |
| RHI 抽象层 | RHIDevice / RHICommandBuffer 等 |
| SyncToScene 桥接 | 游戏侧到渲染侧的数据同步 |

### ⬜ 待改进

| 改进项 | 优先级 | 说明 |
|-------|--------|------|
| Engine 不持有 CommandBuffer | 🔴 高 | CommandBuffer 应由 Renderer 自己管理 |
| SceneRenderer 抽象基类 | 🟠 中 | 区分 Forward / Deferred 渲染路径 |
| FMeshPassProcessor | 🟠 中 | 按 Pass 分离绘制命令（多 Pass 时需要） |
| FSceneViewFamily + 多 View | 🟡 低 | 支持分屏 / 编辑器多视口 |
| DrawCommand 缓存 | 🟡 低 | 静态物体的 DrawCommand 只构建一次 |
| FMeshBatch 中间层 | 🟡 低 | Proxy 和 DrawCommand 之间加入材质批次层 |

---

## 8. 推荐改进路线

### 第一步：让 Engine 不持有 CommandBuffer

```cpp
// 改进前
class Engine {
    RHICommandBuffer* m_CommandBuffer;  // ❌ Engine 不应持有
};

// 改进后
class SceneRenderer {
    void Render(FScene* scene, RHIDevice* device);  // 不再外部传入 cmdBuf
private:
    RHICommandBuffer* m_CmdBuf;  // SceneRenderer 自己持有或从 Device 获取
};
```

### 第二步：引入 SceneRenderer 抽象基类

```cpp
class FSceneRenderer {
public:
    virtual void Render() = 0;
protected:
    FScene* m_Scene;
    FViewInfo m_ViewInfo;
    RHIDevice* m_Device;
};

class FForwardSceneRenderer : public FSceneRenderer { ... };   // 当前实现
class FDeferredSceneRenderer : public FSceneRenderer { ... };  // 未来扩展
```

### 第三步：引入 FMeshPassProcessor

```cpp
class FMeshPassProcessor {
public:
    virtual void AddMeshBatch(const FMeshBatch& batch, FMeshDrawCommand& outCmd) = 0;
};

class FBasePassMeshProcessor : public FMeshPassProcessor { ... };
class FDepthPassMeshProcessor : public FMeshPassProcessor { ... };
```

### 第四步：DrawCommand 缓存

对静态物体的 `FMeshDrawCommand` 做缓存（仅在 Proxy 变化时重建），避免每帧重新收集。

---

## 参考资料

- [UE5 Rendering Architecture (Epic Games 官方文档)](https://docs.unrealengine.com/5.0/en-US/rendering-architecture/)
- [UE5 Mesh Drawing Pipeline](https://docs.unrealengine.com/5.0/en-US/mesh-drawing-pipeline-in-unreal-engine/)
- UE5 源码路径参考：
  - `Engine/Source/Runtime/Renderer/Private/SceneRendering.cpp`
  - `Engine/Source/Runtime/Renderer/Private/DeferredShadingRenderer.cpp`
  - `Engine/Source/Runtime/Renderer/Private/MobileSceneRenderer.cpp`
  - `Engine/Source/Runtime/RHI/Public/RHICommandList.h`

---

更新日期：2026-03-25
