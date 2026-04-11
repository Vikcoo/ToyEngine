# 架构优化复盘：渲染架构对齐 UE5（第一阶段）

## 背景与问题

在本阶段开始前，渲染路径存在以下结构性问题：
- `RenderSceneBridge` 仅做同步直调，增加了装配层级，但没有提供实质隔离收益。
- `FStaticMeshSceneProxy` 在构造时直接创建 Shader/Pipeline/Buffer，实例层与资源层职责混杂。
- `FStaticMeshSceneProxy` 直接依赖 `RHIDevice*`，层次上不利于保持 SceneProxy 轻量。
- 相同 `TStaticMesh` 的多个实例会重复上传 GPU 数据，资源复用效率低。

这些问题会抬高后续演进成本，尤其是在命令队列化、材质系统扩展和实例化渲染上。

## 设计目标

第一阶段目标是先把职责边界拉到更接近 UE5 的方向：
- 去掉无效桥接层，保留清晰接口边界。
- 让 SceneProxy 回归实例级渲染描述，不直接创建 GPU 资源。
- 引入资产级共享 RenderData，避免重复上传网格数据。
- 将 Shader/Pipeline 生命周期从实例层提升到渲染场景层统一管理。

## 方案与取舍

采用“`IRenderScene` + `FScene` 直实现 + 资产级 RenderData”方案：
- `Scene` 模块暴露 `IRenderScene` 接口，`TWorld` 持有接口指针。
- `Renderer::FScene` 直接实现 `IRenderScene`，替代 `RenderSceneBridge`。
- 新增 `FStaticMeshRenderData`，统一持有静态网格 GPU Buffer 与 Section 索引范围。
- `FStaticMeshSceneProxy` 改为只引用 `FStaticMeshRenderData` 与共享 Pipeline。
- `FScene` 内部缓存 `TStaticMesh -> FStaticMeshRenderData`，实现资产级复用。

取舍：
- 当前仍保持单线程同步调用，先收敛边界与资源所有权。
- Pipeline 目前按静态网格通道统一复用，暂未按材质系统细分。

## 本阶段改动范围（2026-04-11）

- Scene 接口层：
  - `Source/Runtime/Scene/Public/RenderScene.h`
  - `Source/Runtime/Scene/Public/PrimitiveComponent.h`
  - `Source/Runtime/Scene/Private/PrimitiveComponent.cpp`
  - `Source/Runtime/Scene/Public/World.h`
  - `Source/Runtime/Scene/Private/World.cpp`
- Renderer 实现层：
  - `Source/Runtime/Renderer/Public/FScene.h`
  - `Source/Runtime/Renderer/Private/FScene.cpp`
  - `Source/Runtime/Renderer/Public/FStaticMeshRenderData.h`
  - `Source/Runtime/Renderer/Private/FStaticMeshRenderData.cpp`
  - `Source/Runtime/Renderer/Public/FStaticMeshSceneProxy.h`
  - `Source/Runtime/Renderer/Private/FStaticMeshSceneProxy.cpp`
  - `Source/Runtime/Renderer/Public/FMeshDrawCommand.h`
  - `Source/Runtime/Renderer/Private/SceneRenderer.cpp`
  - `Source/Runtime/Renderer/CMakeLists.txt`
- Engine 装配层：
  - `Source/Runtime/Engine/Public/Engine.h`
  - `Source/Runtime/Engine/Private/Engine.cpp`

## 效果与收益

- `Bridge` 装配层被移除后，`Engine -> World -> IRenderScene(FScene)` 路径更直接。
- SceneProxy 不再创建 GPU 资源，实例层与资源层职责边界更清晰。
- 相同静态网格实例共享资产级 GPU 数据，消除了重复上传。
- Shader/Pipeline 从实例级迁移到 `FScene` 统一管理，减少重复创建。

## 当前限制与后续方向

- `IRenderScene` 仍是同步调用，尚未命令队列化。
- `FScene` 当前同时承担场景容器与部分资源管理职责，后续可拆分资源子系统。
- 材质系统尚未落地，Pipeline 缓存粒度仍偏粗。
