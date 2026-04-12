# 架构优化复盘：Scene 与 Renderer 解耦（第一阶段）

> 说明：本文档记录 2026-04-10 的第一阶段结果。当前实现已在后续阶段继续演进，见《架构优化复盘：渲染架构对齐UE5第一阶段》。

## 背景与问题

优化前的核心问题是游戏侧直接管理渲染侧生命周期：
- `TPrimitiveComponent` 直接创建和销毁 `FPrimitiveSceneProxy`
- `TWorld` 直接持有 `FScene*` 与 `RHIDevice*`
- `SyncToScene()` 直接操作渲染对象

这种结构在单线程下能工作，但会在后续引入渲染线程、延迟销毁、命令队列时快速放大复杂度。

## 本次设计目标

第一阶段只做边界与所有权纠正，不一次性引入线程模型：
- 游戏侧只依赖桥接接口，不依赖渲染实现类型
- 渲染对象生命周期归渲染侧所有
- 游戏侧通过稳定句柄做增量同步

## 方案与关键取舍

### 方案选择

采用“桥接接口 + 句柄同步 + 渲染侧持有对象”的组合方案：
- `World`（当时目录名为 `Scene`）新增 `IRenderSceneBridge` 接口与 `RenderPrimitiveHandle`
- `TPrimitiveComponent` 从 `CreateSceneProxy` 改为 `BuildRenderCreateInfo`
- `TWorld` 改为持有桥接接口指针
- `FScene` 改为持有 `unique_ptr<FPrimitiveSceneProxy>`
- `Renderer` 提供默认桥接实现 `RenderSceneBridge`

### 关键取舍

- 当前桥接仍是同步直接调用，优先保证边界清晰和行为稳定。
- 暂不引入渲染命令队列，避免第一阶段改动范围过大。

## 变更清单（第一阶段）

- 新增桥接接口：
  - `Source/Runtime/Scene/Public/RenderSceneBridge.h`
- `World` 侧重构：
  - `Source/Runtime/World/Public/PrimitiveComponent.h`
  - `Source/Runtime/World/Private/PrimitiveComponent.cpp`
  - `Source/Runtime/World/Public/MeshComponent.h`
  - `Source/Runtime/World/Private/MeshComponent.cpp`
  - `Source/Runtime/World/Public/World.h`
  - `Source/Runtime/World/Private/World.cpp`
  - `Source/Runtime/World/CMakeLists.txt`
- `Renderer` 侧重构：
  - `Source/Runtime/Renderer/Public/RendererScene.h`
  - `Source/Runtime/Renderer/Private/RendererScene.cpp`
  - `Source/Runtime/Renderer/Public/RenderSceneBridgeFactory.h`
  - `Source/Runtime/Renderer/Private/RenderSceneBridge.cpp`
  - `Source/Runtime/Renderer/CMakeLists.txt`
- 视图信息归位：
  - 新增 `Source/Runtime/World/Public/SceneViewInfo.h`
  - `CameraComponent` 改为依赖 `SceneViewInfo.h`
  - `Renderer/Public/ViewInfo.h` 改为兼容转发头
- `Engine` 装配更新：
  - `Source/Runtime/Engine/Public/Engine.h`
  - `Source/Runtime/Engine/Private/Engine.cpp`

## 改造收益

- `World` 不再直接依赖 `Renderer` 的 Proxy 类型，模块边界方向正确。
- 游戏侧不再直接 `new/delete` 渲染对象，生命周期集中在渲染侧。
- `World` 不再直接持有 `FScene*` 和 `RHIDevice*`，同步职责收敛为桥接接口调用。
- 后续引入命令队列时，可在桥接层演进，不必回到组件层改 API。

## 当前限制与后续建议

- 桥接层仍是同步调用，尚未命令队列化。
- `Engine` 仍负责桥接装配，后续可继续下沉到更独立的 Runtime 装配模块。
- 需要继续收敛 `Renderer` 的 `PUBLIC` 依赖传播，减少下游编译污染。
