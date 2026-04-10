# ToyEngine 一页索引

极简路径表，供快速跳转；说明见 [ARCHITECTURE.md](./ARCHITECTURE.md)。

---

## 入口与构建

| 项 | 路径 |
|----|------|
| 程序入口 | `Source/Sandbox/Main.cpp` |
| 根 CMake | `CMakeLists.txt` |
| Runtime 子模块 | `Source/Runtime/CMakeLists.txt` |
| 构建选项（RHI 等） | `CMake/EngineOptions.cmake` |

---

## 模块 → 主要头文件（Public）

| 模块 | 路径 | 核心类型 |
|------|------|----------|
| Engine | `Source/Runtime/Engine/Public/Engine.h` | `Engine` |
| Platform | `Source/Runtime/Platform/Public/Window.h` | `Window`, `WindowConfig` |
| RHI | `Source/Runtime/RHI/Public/RHI.h` | 聚合头 |
| RHI | `Source/Runtime/RHI/Public/RHIDevice.h` | `RHIDevice` |
| RHI | `Source/Runtime/RHI/Public/RHICommandBuffer.h` | `RHICommandBuffer` |
| RHI | `Source/Runtime/RHI/Public/RHITypes.h` | 枚举与 `*Desc` 结构体 |
| RHI | `Source/Runtime/RHI/Public/RHIBuffer.h` | `RHIBuffer` |
| RHI | `Source/Runtime/RHI/Public/RHIShader.h` | `RHIShader` |
| RHI | `Source/Runtime/RHI/Public/RHIPipeline.h` | `RHIPipeline` |
| OpenGLRHI | `Source/Runtime/OpenGLRHI/Private/OpenGLDevice.h` | `OpenGLDevice` |
| Asset | `Source/Runtime/Asset/Public/TStaticMesh.h` | `TStaticMesh` |
| Asset | `Source/Runtime/Asset/Public/FAssetImporter.h` | `FAssetImporter` |
| Scene | `Source/Runtime/Scene/Public/World.h` | `TWorld` |
| Scene | `Source/Runtime/Scene/Public/Actor.h` | `TActor` |
| Scene | `Source/Runtime/Scene/Public/Component.h` | `TComponent` |
| Scene | `Source/Runtime/Scene/Public/SceneComponent.h` | `TSceneComponent` |
| Scene | `Source/Runtime/Scene/Public/PrimitiveComponent.h` | `TPrimitiveComponent` |
| Scene | `Source/Runtime/Scene/Public/MeshComponent.h` | `TMeshComponent` |
| Scene | `Source/Runtime/Scene/Public/CameraComponent.h` | `TCameraComponent` |
| Renderer | `Source/Runtime/Renderer/Public/FScene.h` | `FScene` |
| Renderer | `Source/Runtime/Renderer/Public/SceneRenderer.h` | `SceneRenderer` |
| Renderer | `Source/Runtime/Renderer/Public/FPrimitiveSceneProxy.h` | `FPrimitiveSceneProxy` |
| Renderer | `Source/Runtime/Renderer/Public/FStaticMeshSceneProxy.h` | `FStaticMeshSceneProxy` |
| Renderer | `Source/Runtime/Renderer/Public/FViewInfo.h` | `FViewInfo` |
| Renderer | `Source/Runtime/Renderer/Public/FMeshDrawCommand.h` | `FMeshDrawCommand` |
| Core | `Source/Runtime/Core/Public/Math/MathTypes.h` | 向量/矩阵/四元数 |
| Core | `Source/Runtime/Core/Public/Math/Transform.h` | `Transform` |
| Core | `Source/Runtime/Core/Public/Log/Log.h` | `Log`, `TE_LOG_*` |
| Core | `Source/Runtime/Core/Public/Memory/Memory.h` | `MemoryInit`, `MemAlloc`… |
| Core | `Source/Runtime/Core/Public/Memory/MemoryNew.h` | `TE::New`, `TE::Delete` |

---

## 测试

| 项 | 路径 |
|----|------|
| 测试 CMake | `Tests/CMakeLists.txt` |

---

## 文档

| 项 | 路径 |
|----|------|
| 架构详解 | [ARCHITECTURE.md](./ARCHITECTURE.md) |
| 本索引 | [INDEX.md](./INDEX.md) |
