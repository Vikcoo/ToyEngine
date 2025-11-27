# ToyEngine 架构设计文档

## 概述

ToyEngine 是一个轻量级游戏引擎，专为个人开发者设计。它借鉴了 Unreal Engine 5、Hazel Engine 等优秀引擎的设计理念，但保持简洁实用的原则。

## 核心设计原则

1. **简单但不简陋** - 保持清晰的分层，但不过度设计
2. **可扩展但不复杂** - 为未来留空间，但不预先实现
3. **模块化但不臃肿** - 清晰的模块边界，但用简单的CMake组织

## 模块说明

### Core（核心模块）

提供引擎的基础设施，包括：

- **Base/** - 基础类型定义
- **Math/** - 数学库封装（基于glm）
- **Memory/** - 内存管理工具
- **FileSystem/** - 文件系统抽象
- **Log/** - 日志系统（基于spdlog）
- **Application/** - 应用程序框架

### Platform（平台抽象层）

提供跨平台的操作系统功能抽象：

- 窗口管理
- 输入处理
- 文件系统接口
- 线程和同步

### RHI（渲染硬件接口）⭐ 核心模块

这是引擎最重要的模块，提供统一的渲染接口抽象：

**设计理念：**
- Public目录只包含纯抽象接口
- Private目录包含具体的图形API实现
- 上层代码完全不知道Vulkan/D3D12的存在

**接口层次：**
```
RHIContext    - 图形上下文
RHIDevice     - 设备（资源工厂）
RHISwapChain  - 交换链
RHICommandBuffer - 命令缓冲
RHIBuffer     - 缓冲区
RHITexture    - 纹理
RHIPipeline   - 管线
RHIShader     - Shader
```

### Renderer（高层渲染器）

基于RHI构建的高层渲染功能：

- 前向渲染器
- 延迟渲染器（可选）
- 相机系统
- 材质系统
- 网格管理

### Asset（资产管理）

负责引擎资源的加载和管理：

- 纹理加载（stb_image）
- 模型加载（tinyobjloader）
- Shader编译和加载
- 资产缓存系统

### Scene（场景系统）

场景图和实体组件系统：

- 场景管理
- 实体系统（可选ECS）
- 组件系统（Transform, Mesh, Camera等）

### Input（输入系统）

输入事件处理和映射。

## 依赖关系

```
    [Application]
         ↓
    [Renderer] ← [Scene]
         ↓           ↓
       [RHI]  ← [Asset]
         ↓
    [Platform] ← [Input]
         ↓
       [Core]
```

## RHI抽象设计详解

### 为什么需要RHI？

1. **跨平台支持** - 同一份代码可以在不同图形API上运行
2. **易于维护** - 图形API更新不影响上层代码
3. **灵活切换** - 运行时可以选择不同的后端

### RHI接口示例

```cpp
// 上层代码示例
auto context = RHIContext::Create(desc);
auto device = context->CreateDevice();
auto swapChain = device->CreateSwapChain(swapChainDesc);
auto pipeline = device->CreateGraphicsPipeline(pipelineDesc);

// 渲染循环
auto cmdBuffer = device->CreateCommandBuffer();
cmdBuffer->Begin();
cmdBuffer->BeginRenderPass(renderPass, framebuffer);
cmdBuffer->BindPipeline(pipeline);
cmdBuffer->Draw(3, 1, 0, 0);
cmdBuffer->EndRenderPass();
cmdBuffer->End();
```

注意：上层代码完全不知道Vulkan的存在！

## Application框架

引擎提供了统一的应用程序框架，简化用户代码：

```cpp
class MyApp : public TE::Application {
public:
    void OnInit() override { /* 初始化 */ }
    void OnUpdate(float dt) override { /* 更新 */ }
    void OnRender() override { /* 渲染 */ }
};

TE_CREATE_APPLICATION(MyApp)
```

## 开发阶段规划

### 阶段1：核心基础（2-4周）
- Core模块基础功能
- Platform模块基本实现
- RHI抽象接口定义
- Vulkan后端实现

### 阶段2：渲染基础（2-3周）
- 基础Renderer实现
- Mesh、Material、Texture类
- 简单的前向渲染器
- Camera系统

### 阶段3：资产和场景（2-3周）
- AssetManager实现
- 纹理和模型加载
- Scene系统
- 组件系统

### 阶段4：高级特性（按需）
- PBR材质系统
- 阴影渲染
- ImGui编辑器集成
- 场景序列化

## 参考资料

- **Unreal Engine 5** - RHI抽象设计
- **Hazel Engine** - 整体架构参考
- **The-Forge** - 多后端RHI实现
- **bgfx** - 跨平台渲染抽象

---

更新日期：2025-11-24

