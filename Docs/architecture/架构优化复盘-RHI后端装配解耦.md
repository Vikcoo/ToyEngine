# 架构优化复盘：RHI 后端装配解耦（第一阶段）

## 背景与问题

在优化前，`RHI` 模块存在“抽象层反向依赖具体后端”的问题：
- `RHI` 会直接链接 `OpenGLRHI`
- `RHI` 工厂实现会直接包含 `OpenGLRHI/Private/OpenGLDevice.h`

这导致模块边界方向错误：理论上应由外层装配模块依赖抽象层和具体后端，而不是抽象层反向依赖实现层。

## 本次设计目标

本次重构的目标不是一次性做完全部后端系统，而是完成第一阶段的边界纠正：
- 让 `RHI` 只保留抽象接口，不直接依赖任何具体后端
- 将具体后端创建逻辑移动到更外层模块
- 避免外层模块直接包含后端私有头

## 方案与取舍

### 方案选择

采用“后端提供公开装配入口，外层负责调用装配入口”的方式：
- 在 `OpenGLRHI/Public` 增加 `CreateOpenGLRHIDevice()` 声明
- 在 `OpenGLRHI/Private` 实现该入口并创建 `OpenGLDevice`
- 在 `Engine` 新增 `RHIDevice::Create()` 实现，按编译开关选择后端入口

### 关键取舍

- 当前先把装配逻辑放在 `Engine`，优点是改动面最小、可快速落地并验证。
- 该选择也意味着 `Engine` 仍承担一部分装配责任。后续如果需要进一步收敛 `Engine` 职责，可再抽出专门装配模块（例如 RuntimeBootstrap 或 RHIRuntime）。

## 变更清单（第一阶段）

- 删除 `RHI` 模块内的后端工厂实现：
  - `Source/Runtime/RHI/Private/RHIFactory.cpp`
- 收敛 `RHI` 模块依赖：
  - `Source/Runtime/RHI/CMakeLists.txt`
- 新增 OpenGL 后端公开装配入口：
  - `Source/Runtime/OpenGLRHI/Public/OpenGLRHIEntry.h`
  - `Source/Runtime/OpenGLRHI/Private/OpenGLRHIEntry.cpp`
- 将 `RHIDevice::Create()` 实现迁移到外层装配：
  - `Source/Runtime/Engine/Private/RHIDeviceFactory.cpp`
  - `Source/Runtime/Engine/CMakeLists.txt`

## 改造收益

- `RHI` 抽象层边界变清晰：不再直接依赖 `OpenGLRHI`。
- 后端私有头不再被抽象层引用，后端实现细节收敛在后端模块内部。
- 后续接入 Vulkan / D3D12 时，可以复用同一模式（后端公开装配入口 + 外层装配）。
- 减少了“抽象层修改牵连具体后端”或“后端细节污染抽象层”的风险。

## 当前限制与后续建议

- 当前仅完成 OpenGL 路径的第一阶段拆分，尚未形成统一的多后端装配框架。
- `Engine` 仍承担后端选择逻辑；若未来要支持更多运行入口（编辑器、工具链、离线程序），建议将装配逻辑进一步抽离。
- 当 Vulkan / D3D12 开始接入时，应同时补齐对应 `Create*RHIDevice()` 入口，避免回退到抽象层反向依赖模式。
