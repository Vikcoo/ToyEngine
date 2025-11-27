# Renderer 模块

高层渲染器，基于RHI提供易用的渲染功能。

## 📁 目录结构

- **Public/**
  - `Renderer.h` - 渲染器主类（静态接口）
  - `RenderCommand.h` - 渲染命令（立即执行）
  - `Camera.h` - 相机类
  - `Mesh.h` - 网格类
  - `Material.h` - 材质类
  - `Shader.h` - Shader资源（高层）
  - `Texture2D.h` - 纹理资源（高层）
  - `VertexArray.h` - 顶点数组（简化VAO概念）

- **Private/**
  - `Renderer.cpp` - 渲染器实现
  - `ForwardRenderer.cpp` - 前向渲染器
  - `RenderGraph.cpp` - 渲染图（简化版）

## 🎯 设计目标

1. **封装RHI细节** - 用户不直接操作CommandBuffer
2. **提交式渲染** - 类似Unity的`Graphics.DrawMesh`
3. **易于使用** - 简洁的API设计

## 📝 使用示例

```cpp
#include "Renderer/Public/Renderer.h"

using namespace TE;

// 初始化渲染器
Renderer::Init();

// 渲染循环
while (running) {
    RenderCommand::Clear(0.1f, 0.1f, 0.1f);
    
    Renderer::BeginScene(camera);
    
    // 提交渲染对象
    Renderer::Submit(mesh, material, transform);
    
    Renderer::EndScene();
}

Renderer::Shutdown();
```

## 🏗️ 架构层次

```
Application
    ↓
Renderer（提交式API）
    ↓
RHI（命令缓冲）
```

## 🔗 依赖

- RHI模块
- Asset模块
- Core模块

## ✅ 实现清单

- [ ] Renderer主类
- [ ] RenderCommand
- [ ] Camera系统
- [ ] Mesh类
- [ ] Material系统
- [ ] Shader资源管理
- [ ] Texture2D
- [ ] 前向渲染器
- [ ] 批量渲染优化

