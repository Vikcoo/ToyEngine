# 架构优化复盘：目录结构对齐 UE5（第一阶段）

## 背景与问题

在本阶段开始前，运行时目录中存在一个容易持续制造歧义的命名：
- `Source/Runtime/Scene` 实际承载的是游戏侧 `World/Actor/Component` 体系。
- `Renderer` 中同时存在渲染侧 `FScene`。

这会带来两类问题：
- 目录语义和类语义冲突。开发时很容易把“Scene 模块”和“FScene”误当成同一层概念。
- 随着 `CreateSceneProxy`、`FScene`、`SceneProxy` 这些 UE 风格概念逐步落地，`Scene` 目录名会越来越偏离其真实职责。

## 本次调整目标

本次只调整项目组织结构，不修改运行时类设计和逻辑行为：
- 让游戏侧目录命名更贴近其真实职责。
- 让渲染侧 `FScene` 保留“渲染场景”这一专属语义。
- 尽量不触碰源码中的类名、头文件名和运行时接口。

## 方案与取舍

采用“目录改名 + 构建入口同步 + 文档同步”的轻量方案：
- 将 `Source/Runtime/Scene` 重命名为 `Source/Runtime/World`。
- CMake 目标名同步从 `Scene` 调整为 `World`。
- 保持 `TSceneComponent`、`SceneViewInfo`、`FScene` 等现有类型名不变，避免把一次目录整理扩大成 API 重命名。

取舍：
- 这次并没有把游戏侧对象并入现有 `Engine` 模块。那样会更像 UE5 的大模块组织，但会显著扩大改动范围。
- 当前优先解决“目录名误导”问题，而不是一步到位复制 UE5 的完整模块划分。

## 第一阶段变更范围（2026-04-12）

- `Source/Runtime/World/`
- `Source/Runtime/CMakeLists.txt`
- `Source/Runtime/Renderer/CMakeLists.txt`
- `Source/Runtime/Engine/CMakeLists.txt`
- `Source/Sandbox/CMakeLists.txt`
- `Docs/文档总览.md`
- `Docs/architecture/运行时模块.md`
- `Docs/reference/项目目录结构.md`

## 收益

- 游戏侧目录语义从抽象的 `Scene` 收敛为更直接的 `World`，和渲染侧 `FScene` 明确区分。
- 项目结构更接近 UE 风格里的“游戏世界对象”和“渲染场景对象”分层方式。
- 目录层面不再把“世界对象容器”和“渲染场景容器”混在同一个词上，后续阅读成本更低。

## 当前限制与后续方向

- 现有游戏侧模块从职责上仍然更接近 UE5 的 `Engine` 大模块，只是当前项目里暂时独立成 `World` 模块。
- `TSceneComponent`、`SceneViewInfo` 等类型名仍保留旧命名，这是刻意控制改动范围的结果。
- 如果后续继续向 UE5 的模块组织靠拢，可以再评估：
  - 是否把 `World/Actor/Component` 体系进一步并入更大的 `Engine` 运行时模块。
  - 是否把 `Camera/FlyCameraController` 一类更偏玩法层的内容再做二次分组。

## 增量记录：第二阶段（2026-04-12）

### 为什么继续改

虽然上一阶段已经把游戏侧目录从 `Scene` 调整为 `World`，但 RHI 相关目录仍保留了旧的“后端名 + RHI”混合风格：
- `RHI`
- `OpenGLRHI`
- `VulkanRHI`
- `D3D12RHI`

这种结构在依赖方向上没有问题，但命名风格不统一，不利于把 `RHI` 抽象层和各后端实现层表达成一组平级模块。

### 第二阶段核心改动

- 将 RHI 后端目录统一重命名为：
  - `Source/Runtime/RHIOpenGL`
  - `Source/Runtime/RHIVulkan`
  - `Source/Runtime/RHID3D12`
- 同步将 OpenGL 后端的 CMake 目标从 `OpenGLRHI` 调整为 `RHIOpenGL`
- 更新 Runtime 构建入口、`Engine` 链接关系和目录文档说明
- 同步清理公开入口命名，例如：
  - `RHIOpenGLEntry.h`
  - `CreateRHIOpenGLDevice()`

### 收益

- `RHI` 与各具体后端现在形成了更统一的平级命名组，目录结构更接近成熟引擎里“抽象层 + 平级后端实现层”的组织方式。
- 避免把 `OpenGLRHI` 这类名字误读为“RHI 模块内部子目录”或“RHI 的一部分命名空间”。
- 接口入口名、目录名、目标名和日志前缀现在统一收敛到 `RHIOpenGL` 命名体系。

### 仍然存在的限制

- 后端内部具体类型仍保留 `OpenGL*` 前缀，例如 `OpenGLDevice`、`OpenGLPipeline`，这是合理的，因为它们描述的是 OpenGL 实现对象，而不是模块名。
- 等 Vulkan / D3D12 真正开始落地时，需要同步为它们定义与模块名一致的公开入口命名。
