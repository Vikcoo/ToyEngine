# ToyEngine 模型加载系统 — 设计思想文档

## 概述

本文档说明 ToyEngine 模型加载与渲染管线的设计思想，帮助你理解整个系统的架构和 UE5 相关概念。

---

## 1. 核心数据流

从一个模型文件到最终渲染到屏幕，经历以下完整的数据流：

```
模型文件 (.obj / .fbx / .gltf)
  │
  ▼
FAssetImporter::ImportStaticMesh()      [Asset 模块]
  │  · 调用 Assimp 加载模型
  │  · 递归遍历场景节点树
  │  · 每个 aiMesh → FMeshSection
  │  · 组装 TStaticMesh 资产
  ▼
TStaticMesh (CPU 侧资产数据)            [Asset 模块]
  │  · 持有 vector<FMeshSection>
  │  · 每个 Section: 顶点数组 + 索引数组
  │  · 可被多个 Component 共享引用
  ▼
TMeshComponent::SetStaticMesh()         [Scene 模块]
  │  · 持有 shared_ptr<TStaticMesh>
  │  · 不持有 GPU 资源
  ▼
TMeshComponent::CreateSceneProxy()      [Scene → Renderer 桥接]
  │  · 读取 TStaticMesh 数据
  │  · 通过 RHIDevice 创建 GPU 资源
  ▼
FStaticMeshSceneProxy                   [Renderer 模块]
  │  · 每个 Section → VBO + IBO
  │  · 共享 Shader + Pipeline
  │  · GetMeshDrawCommands() → 多条 FMeshDrawCommand
  ▼
SceneRenderer::Render()                 [Renderer 模块]
  │  · 遍历 FScene 中所有 Proxy
  │  · 收集 MeshDrawCommands
  │  · 通过 RHI CommandBuffer 提交绘制
  ▼
RHI CommandBuffer → OpenGL 调用         [RHI / OpenGLRHI 模块]
  │  · glBindPipeline, glBindVertexBuffer, etc.
  │  · glDrawElements
  ▼
屏幕像素输出
```

---

## 2. UE5 概念映射

| UE5 概念 | ToyEngine 对应 | 说明 |
|---------|---------------|------|
| **UStaticMesh** | `TStaticMesh` | 静态网格资产，持有 CPU 侧顶点/索引数据 |
| **FStaticMeshLODResources** | `FMeshSection` | 一个 LOD 级别的一个 Section（子网格） |
| **UStaticMeshComponent** | `TMeshComponent` | 游戏侧组件，引用 TStaticMesh 资产 |
| **FStaticMeshSceneProxy** | `FStaticMeshSceneProxy` | 渲染侧镜像，持有 GPU 资源 |
| **FMeshDrawCommand** | `FMeshDrawCommand` | 一条绘制命令（Pipeline + VBO + IBO + Uniform） |
| **FSceneRenderer** | `SceneRenderer` | 渲染调度器，遍历 Proxy、收集和提交 DrawCommand |
| **FScene** | `FScene` | 渲染场景容器，存储所有 Proxy 和 ViewInfo |
| **FAssetRegistry** | `FAssetImporter` | 资产发现/加载（极简版） |
| **FSceneView / FViewInfo** | `FViewInfo` | 相机视图信息（View/Projection 矩阵） |

---

## 3. 模块划分思想

### 3.1 五大核心模块

```
┌──────────────────────────────────────────────────┐
│                     Engine                        │
│  · 引擎主类，协调所有子系统                         │
│  · 持有 World、FScene、SceneRenderer               │
│  · 管理主循环：Tick → Sync → Render → Swap         │
├──────────────────────────────────────────────────┤
│                                                   │
│  ┌──────────┐  ┌──────────┐  ┌───────────────┐   │
│  │  Asset    │  │  Scene   │  │   Renderer    │   │
│  │          │  │          │  │               │   │
│  │ TStatic  │←─│ TMesh    │──→FStaticMesh    │   │
│  │   Mesh   │  │ Component│  │  SceneProxy   │   │
│  │ FAsset   │  │ TActor   │  │ SceneRenderer │   │
│  │ Importer │  │ TWorld   │  │ FScene        │   │
│  └──────────┘  └──────────┘  └───────────────┘   │
│        │                           │              │
│        └───────────┬───────────────┘              │
│                    ▼                              │
│              ┌──────────┐                         │
│              │   RHI    │                         │
│              │ 图形API  │                         │
│              │ 抽象层   │                         │
│              └──────────┘                         │
│                    │                              │
│              ┌──────────┐                         │
│              │OpenGLRHI │                         │
│              │ OpenGL   │                         │
│              │ 后端实现  │                         │
│              └──────────┘                         │
└──────────────────────────────────────────────────┘
```

### 3.2 关键设计原则

**① 资产与组件分离（对应 UE5 的 UStaticMesh vs UStaticMeshComponent）**

- `TStaticMesh` 是**资产**——持有数据，可被多个组件共享
- `TMeshComponent` 是**组件**——引用资产，负责在场景中放置
- 好处：同一个模型可以被放置多次，不重复存储顶点数据

**② 游戏侧与渲染侧分离（对应 UE5 的 Game Thread vs Render Thread）**

- 游戏侧（Scene 模块）：`TWorld` → `TActor` → `TMeshComponent`
- 渲染侧（Renderer 模块）：`FScene` → `FPrimitiveSceneProxy` → `FMeshDrawCommand`
- 桥接：`CreateSceneProxy()` 创建镜像，`SyncToScene()` 同步 Transform

**③ Assimp 头文件隔离**

- `FAssetImporter.h`（Public）只暴露引擎自有类型
- `FAssetImporter.cpp`（Private）才 `#include <assimp/...>`
- CMake 中 assimp 以 `PRIVATE` 链接到 Asset 模块
- 好处：其他模块不需要知道 Assimp 的存在，换用其他加载库时只改 Asset 模块

---

## 4. 顶点数据设计

### FStaticMeshVertex 布局

```cpp
struct FStaticMeshVertex
{
    Vector3 Position;   // 位置     (12 bytes, location=0)
    Vector3 Normal;     // 法线     (12 bytes, location=1)
    Vector2 TexCoord;   // 纹理坐标 (8 bytes,  location=2)
    Vector3 Color;      // 顶点色   (12 bytes, location=3)
    // Total: 44 bytes per vertex
};
```

与 Shader 的对应关系：

```glsl
layout(location = 0) in vec3 aPosition;   // FStaticMeshVertex::Position
layout(location = 1) in vec3 aNormal;     // FStaticMeshVertex::Normal
layout(location = 2) in vec2 aTexCoord;   // FStaticMeshVertex::TexCoord
layout(location = 3) in vec3 aColor;      // FStaticMeshVertex::Color
```

### 缺失属性的 Fallback 策略

| 属性 | 缺失时的默认值 | 原因 |
|------|-------------|------|
| Normal | (0, 0, 1) | 朝向 Z+ 的默认法线 |
| TexCoord | (0, 0) | 纹理坐标原点 |
| Color | (1, 1, 1) 白色 | 白色不影响后续材质颜色 |

---

## 5. 光照模型

使用 **Lambertian 漫反射** + **环境光** 的简单方向光模型：

```
最终颜色 = (ambient + diffuse × NdotL) × vertexColor

其中:
  ambient  = 0.15 × lightColor     // 环境光（防止全黑）
  NdotL    = max(dot(N, L), 0)      // 法线与光线方向的点积
  diffuse  = NdotL × lightColor     // Lambertian 漫反射
```

Uniform 参数：

| Uniform 名 | 类型 | 说明 |
|-----------|------|------|
| `u_MVP` | mat4 | Model-View-Projection 变换矩阵 |
| `u_Model` | mat4 | Model 矩阵（用于法线变换到世界空间） |
| `u_LightDir` | vec3 | 方向光方向（指向光源，已归一化） |
| `u_LightColor` | vec3 | 光源颜色 |

---

## 6. Assimp 后处理标志

```cpp
aiProcess_Triangulate           // 将所有多边形面转为三角形（必须）
aiProcess_GenSmoothNormals      // 为缺失法线的网格生成平滑法线
aiProcess_FlipUVs               // 翻转 UV 的 Y 轴（适配 OpenGL）
aiProcess_JoinIdenticalVertices // 合并相同顶点，减少冗余
aiProcess_CalcTangentSpace      // 计算切线空间（为法线贴图预留）
```

---

## 7. 多 Section 机制

一个模型文件可能包含多个子网格（Mesh），在 UE5 中叫 **Section**，每个 Section 通常对应一个材质。

```
模型文件 (e.g. character.fbx)
  ├── Section 0: 身体   (Material: skin)
  ├── Section 1: 武器   (Material: metal)
  └── Section 2: 头发   (Material: hair)
```

在 ToyEngine 中：
- `TStaticMesh` 持有 `vector<FMeshSection>`
- `FStaticMeshSceneProxy` 为每个 Section 创建独立的 VBO/IBO
- `GetMeshDrawCommands()` 返回多条 `FMeshDrawCommand`（每个 Section 一条）
- SceneRenderer 遍历所有命令统一提交

---

## 8. 生命周期管理

```
创建阶段:
  FAssetImporter → TStaticMesh (shared_ptr, Engine 持有)
  TMeshComponent → SetStaticMesh(shared_ptr 引用)
  TMeshComponent::CreateSceneProxy() → FStaticMeshSceneProxy (new)
  FScene::AddPrimitive(proxy) → 注册到渲染场景

每帧更新:
  World::Tick() → 更新 Actor Transform
  World::SyncToScene() → 脏 Component 同步 WorldMatrix 到 Proxy
  SceneRenderer::Render() → 遍历 Proxy → DrawCommand → RHI 提交

销毁阶段:
  World.reset() → TActor 析构 → TMeshComponent 析构
    → UnregisterFromScene() → FScene::RemovePrimitive()
    → delete Proxy (GPU 资源在 Proxy 析构时释放)
  Engine::m_LoadedMesh.reset() → TStaticMesh 析构 (CPU 数据释放)
```

---

## 9. 如何使用

### 加载外部模型

将 `.obj`、`.fbx`、`.gltf` 等模型文件放入 `Content/Models/` 目录，命名为：
- `model.obj`
- `model.fbx`
- `model.gltf`
- `model.glb`

引擎启动时会自动扫描这些文件并加载第一个找到的模型。

### 如果没有外部模型

引擎会自动 fallback 到一个内置的彩色立方体（带正确法线），可以看到方向光照效果。

### 代码中手动加载

```cpp
// 加载模型
auto mesh = FAssetImporter::ImportStaticMesh("/path/to/model.obj");

// 创建 Actor + Component
auto actor = std::make_unique<TActor>();
auto* comp = actor->AddComponent<TMeshComponent>();
comp->SetStaticMesh(mesh);

// 添加到 World
m_World->AddActor(std::move(actor));
```

---

## 10. 扩展方向

| 方向 | 说明 |
|------|------|
| **材质系统** | 引入 `TMaterial` 资产，每个 Section 引用不同材质 |
| **纹理贴图** | 扩展 FStaticMeshVertex 或 Material 支持 Diffuse/Normal 纹理 |
| **LOD 系统** | TStaticMesh 持有多级 LOD 数据，根据距离切换 |
| **异步加载** | 在后台线程加载模型，完成后通知主线程 |
| **资产管理器** | 实现真正的 AssetManager，支持路径查找、缓存、引用计数 |
| **实例化渲染** | 相同 Mesh 的多个 DrawCommand 合并为 Instanced Draw |
| **骨骼动画** | 引入 SkeletalMesh / Animation 资产 |
