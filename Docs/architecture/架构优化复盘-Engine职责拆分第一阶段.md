# 架构优化复盘：Engine 职责拆分（第一阶段）

## 背景与问题

在本阶段开始前，`Engine` 同时承担了两类职责：
- 引擎内核职责：子系统装配、主循环、渲染调度、关闭顺序
- 示例应用职责：模型加载、默认立方体生成、场景搭建、示例旋转逻辑

这会导致内核层和产品层耦合，后续扩展多个示例入口、测试入口或编辑器入口时，变更会集中压在 `Engine`。

## 方案与取舍

第一阶段采用“内核保留 + 应用注入”的拆分方案：
- `Engine` 保留：初始化、主循环、`World/Scene/Renderer` 调度、FPS 统计
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
