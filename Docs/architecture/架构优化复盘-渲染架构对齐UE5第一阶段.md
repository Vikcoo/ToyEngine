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
  - `Source/Runtime/World/Public/RenderScene.h`
  - `Source/Runtime/World/Public/PrimitiveComponent.h`
  - `Source/Runtime/World/Private/PrimitiveComponent.cpp`
  - `Source/Runtime/World/Public/World.h`
  - `Source/Runtime/World/Private/World.cpp`
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

## 增量记录：第二阶段（2026-04-12）

### 为什么继续改

第一阶段虽然清理了资源职责，但 `IRenderScene` 仍以“创建返回句柄 + 句柄更新/销毁”为主，这与 UE5 更常见的 `CreateSceneProxy` + `AddPrimitive/RemovePrimitive` 心智模型仍有差距。

### 第二阶段核心改动

- `TPrimitiveComponent` 从 `BuildRenderCreateInfo` 语义调整为 `CreateSceneProxy` 语义。
- 移除 `RenderPrimitiveHandle`，改为以 `TPrimitiveComponent*` 作为渲染侧注册键。
- `IRenderScene` 接口调整为：
  - `AddPrimitive(const TPrimitiveComponent*, std::unique_ptr<FPrimitiveSceneProxy>) -> bool`
  - `UpdatePrimitiveTransform(const TPrimitiveComponent*, const Matrix4&)`
  - `RemovePrimitive(const TPrimitiveComponent*)`
- `FScene` 内部存储从 `handle -> proxy` 改为 `primitive component -> proxy`。
- `TWorld::SyncToScene` 改为按组件指针直接更新变换。

### 收益

- 接口语义更接近 UE5 的 `CreateSceneProxy` / `AddPrimitive` 设计习惯。
- 去掉句柄分配与映射层后，同步路径更直接，调试时更容易建立“组件-代理”对应关系。
- 后续引入渲染线程命令队列时，可直接以组件标识组织命令，不再额外维护句柄生命周期。

### 仍然存在的限制

- 当前仍是单线程直调，尚未进入真正的 GT/RT 命令队列模型。
- 组件指针作为键在当前单线程生命周期下可行，未来跨线程阶段仍需引入稳定 ID 以避免悬挂指针风险。

## 增量记录：第三阶段（2026-04-12）

### 为什么继续改

第二阶段虽然把接口名字和注册语义拉近了 UE5，但 `CreateSceneProxy` 仍只是“组件填充创建参数，`FScene` 再 switch 创建具体代理”，本质上还是渲染场景在决定代理类型，这与 UE5 中组件直接 override `CreateSceneProxy()` 的职责分配仍不一致。

### 第三阶段核心改动

- 新增 `RenderCore` 模块，承载共享渲染类型：
  - `FPrimitiveSceneProxy`
  - `FStaticMeshSceneProxy`
  - `FStaticMeshRenderData`
  - `FMeshDrawCommand`
- `Scene` 模块改为依赖 `RenderCore`，使 `TPrimitiveComponent` / `TMeshComponent` 可以直接声明并返回具体 `SceneProxy` 类型。
- `TPrimitiveComponent::CreateSceneProxy` 调整为：
  - `std::unique_ptr<FPrimitiveSceneProxy> CreateSceneProxy(IRenderScene&) const`
- `TMeshComponent::CreateSceneProxy` 直接从 `IRenderScene` 获取共享 `RenderData` 与共享 Pipeline，并直接构造 `FStaticMeshSceneProxy`。
- `IRenderScene` 补充共享资源查询接口：
  - `GetStaticMeshRenderData(const std::shared_ptr<TStaticMesh>&)`
  - `GetStaticMeshPipeline()`
- `FScene::AddPrimitive` 收缩为“接收并注册已创建好的代理”，不再承担 `CreateInfo + switch` 工厂职责。

### 收益

- 组件侧真正具备了与 UE5 更接近的 `CreateSceneProxy()` override 语义，而不是旧工厂接口换名。
- `FScene` 的职责收敛为场景容器、共享资源提供者和同步入口，边界更清晰。
- 通过单独的 `RenderCore` 模块承载共享渲染类型，避免为了对齐 UE5 而重新把 `Scene` 与 `Renderer` 紧耦合。

### 仍然存在的限制

- `IRenderScene` 现在同时承担“场景注册接口”和“共享渲染资源查询接口”，未来仍可继续拆出更细的资源服务边界。
- 当前 `TMeshComponent` 仍直接请求静态网格通道的共享 Pipeline，这在材质系统落地前是合理的简化，但还不是 UE5 那种由材质/顶点工厂共同决定绘制状态的完整模型。
