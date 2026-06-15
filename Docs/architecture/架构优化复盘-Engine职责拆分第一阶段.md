# 架构优化复盘：Engine 职责拆分（第一阶段）

## 背景与问题

在本阶段开始前，`Engine` 同时承担了两类职责：
- 引擎内核职责：子系统装配、主循环、渲染调度、关闭顺序
- 示例应用职责：模型加载、默认立方体生成、场景搭建、示例旋转逻辑

这会导致内核层和产品层耦合，后续扩展多个示例入口、测试入口或编辑器入口时，变更会集中压在 `Engine`。

## 方案与取舍

第一阶段采用“内核保留 + 应用注入”的拆分方案：
- `Engine` 保留：初始化、主循环、`World/Renderer` 调度、FPS 统计
- `Engine` 新增应用注入点：
  - `SetSceneSetupCallback`
  - `SetFrameUpdateCallback`
  - `SetActiveCameraComponent`
- `Sandbox` 接管示例场景与示例逻辑：
  - 场景搭建与资产导入在 `SceneSetupCallback` 中完成
  - 示例旋转逻辑在 `FrameUpdateCallback` 中完成

选择回调注入而不是一次性引入完整 `Application` 抽象，主要是为了在最小改动下先完成职责边界收敛，降低第一阶段改造风险。

## 第一阶段变更范围（2026-04-11）

- `Source/Runtime/Engine/Public/Engine.h`
- `Source/Runtime/Engine/Private/Engine.cpp`
- `Source/Runtime/Engine/CMakeLists.txt`
- `Source/Sandbox/Main.cpp`
- `Source/Sandbox/CMakeLists.txt`

## 收益

- `Engine` 从“内核 + 示例”回到“内核调度”定位，模块边界更清晰。
- 示例逻辑迁移到 `Sandbox` 后，可独立替换或扩展不同应用入口。
- 后续做测试场景、基准场景、编辑器入口时，不需要修改 `Engine` 核心流程。

## 限制与后续方向

- 当前仍是函数回调注入，生命周期表达能力有限。
- `Engine` 仍为单例，跨实例测试隔离能力有限。

后续可在不破坏本阶段边界的前提下继续演进：
1. 引入显式 `Application` 接口（Startup/Update/Shutdown）。
2. 评估逐步弱化单例依赖。

## 第二阶段增量：单线程帧阶段拆分（2026-06-16）

### 背景与问题

第一阶段后，`Engine` 已经不再直接包含示例场景逻辑，但 `Engine::Tick` 内部仍把平台事件、输入推进、应用回调、World Tick、游戏侧到渲染侧同步、渲染提交、Present 和统计输出写在同一个函数里。

这在当前规模下可以工作，但会带来两个后续问题：
- 单帧顺序依赖只能靠注释维护，新增调试、编辑器、异步资源或渲染线程准备逻辑时容易插错位置。
- `World::SyncToScene()` 作为游戏侧到渲染侧边界不够醒目，后续命令队列化时缺少清晰替换点。

### 方案与取舍

第二阶段采用“阶段函数拆分，不引入完整 Tick 系统”的方案。`Engine::Tick` 现在只负责编排以下阶段：
- `PumpPlatformMessages()`
- `TickInput(deltaTime)`
- `TickGameThread(deltaTime)`
- `SendAllEndOfFrameUpdates()`
- `TickRenderThread(deltaTime)`
- `EndFrame(deltaTime)`

其中 `TickGameThread` / `TickRenderThread` 借鉴 UE5 的职责命名，但当前仍运行在同一个主线程中。这样做的取舍是：先把单帧职责边界稳定下来，避免过早引入 `TickTaskManager`、TickGroup 或多线程调度的复杂度。

### 收益

- `Engine::Tick` 从具体执行细节变成帧管线编排入口，后续新增阶段时更容易判断插入点。
- `SendAllEndOfFrameUpdates()` 明确承载游戏侧状态提交到渲染侧的语义，后续可自然替换为渲染命令队列提交。
- `EndFrame()` 把输入过渡态清理、Present 和统计输出收拢到帧尾，避免这些维护性逻辑散落在渲染流程中。

### 限制与后续方向

- 当前只是函数级拆分，没有改变 Actor / Component 的 Tick 注册模型。
- `TickGameThread` 和 `TickRenderThread` 只是阶段名，不代表已经有独立线程。
- 后续如果引入物理、动画、编辑器或 UI，再评估是否需要 UE 风格的 TickGroup；在那之前不建议引入完整 Tick 调度系统。
