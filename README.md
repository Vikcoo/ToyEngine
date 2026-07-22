# ToyEngine

一个用于学习引擎架构的玩具引擎项目。

## 文档入口

- 项目文档总入口：`Docs/文档总览.md`
- 架构总览：`Docs/architecture/项目概览.md`
- 构建与运行：`Docs/guides/构建与运行.md`
- 项目结构参考：`Docs/reference/项目目录结构.md`

## 当前实现概览

- CMake 工程，当前配置要求 `CMake 3.18+`
- 项目使用 `C++20`
- 可在配置期互斥选择 `OpenGL` 或 `Vulkan`；OpenGL 提供完整 Forward/Deferred 场景渲染，Vulkan 当前完成阶段 B 的基础资源、Descriptor/Pipeline 与静态网格验证绘制
- `Source/Sandbox/Main.cpp` 是当前可执行入口
- 项目采用单线程运行流，但已经显式区分游戏侧对象与渲染侧对象

**作者**: YuKai  
**开始日期**: 2025-07-23
