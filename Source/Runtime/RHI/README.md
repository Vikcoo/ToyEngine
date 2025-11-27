# RHI 模块 (Rendering Hardware Interface)

渲染硬件接口层，提供统一的图形API抽象。

## 🎯 设计理念

**核心原则：上层代码完全不知道Vulkan/D3D12的存在！**

- **Public/** 只包含纯抽象接口
- **Private/** 包含具体的图形API实现
- 支持多种图形API：Vulkan、DirectX 12、OpenGL

## 📁 目录结构

### Public/（抽象接口层）

- `RHICore.h` - 一站式引入头文件
- `RHIDefinitions.h` - 枚举、结构体定义
- `RHIContext.h` - 图形上下文抽象
- `RHIDevice.h` - 设备抽象（资源工厂）
- `RHISwapChain.h` - 交换链抽象
- `RHICommandBuffer.h` - 命令缓冲抽象
- `RHIBuffer.h` - 缓冲区抽象
- `RHITexture.h` - 纹理抽象
- `RHIPipeline.h` - 管线抽象
- `RHIShader.h` - Shader抽象
- `RHIRenderPass.h` - RenderPass抽象
- `RHIFramebuffer.h` - Framebuffer抽象

### Private/（实现层）

- **Vulkan/** - Vulkan后端实现
  - `VulkanContext.h/cpp`
  - `VulkanDevice.h/cpp`
  - `VulkanSwapChain.h/cpp`
  - ...（其他Vulkan实现）
  
- **D3D12/** - DirectX 12后端实现
  - `D3D12Context.h/cpp`
  - ...（待实现）

## 📝 使用示例

```cpp
#include "RHI/Public/RHICore.h"

using namespace TE;

// 创建RHI上下文
RHIContextDesc contextDesc;
contextDesc.window = window;
contextDesc.api = GraphicsAPI::Vulkan;
auto context = RHIContext::Create(contextDesc);

// 创建设备
auto device = context->CreateDevice();

// 创建交换链
SwapChainDesc swapChainDesc;
swapChainDesc.width = 1280;
swapChainDesc.height = 720;
auto swapChain = device->CreateSwapChain(swapChainDesc);

// 创建命令缓冲
auto cmdBuffer = device->CreateCommandBuffer();
cmdBuffer->Begin();
cmdBuffer->BeginRenderPass(renderPass, framebuffer);
cmdBuffer->BindPipeline(pipeline);
cmdBuffer->Draw(3, 1, 0, 0);
cmdBuffer->EndRenderPass();
cmdBuffer->End();
```

## 🏗️ 架构层次

```
Application/Renderer（上层）
        ↓
   RHI抽象层（Public）
        ↓
 Vulkan | D3D12 | OpenGL（Private）
```

## 🔗 依赖

- Vulkan SDK (如果启用Vulkan)
- Platform模块
- Core模块

## ✅ 实现清单

### 接口定义
- [ ] RHIDefinitions.h（枚举和结构体）
- [ ] RHIContext.h（上下文接口）
- [ ] RHIDevice.h（设备接口）
- [ ] RHISwapChain.h
- [ ] RHICommandBuffer.h
- [ ] RHIBuffer.h
- [ ] RHITexture.h
- [ ] RHIPipeline.h
- [ ] RHIShader.h
- [ ] RHIRenderPass.h
- [ ] RHIFramebuffer.h

### Vulkan实现
- [ ] VulkanContext（Instance、Surface、PhysicalDevice）
- [ ] VulkanDevice（逻辑设备、队列）
- [ ] VulkanSwapChain
- [ ] VulkanCommandBuffer
- [ ] VulkanBuffer
- [ ] VulkanTexture
- [ ] VulkanPipeline
- [ ] VulkanShader
- [ ] VulkanRenderPass
- [ ] VulkanFramebuffer

## 📖 参考资料

- Unreal Engine 5 RHI设计
- The-Forge多后端实现
- bgfx跨平台渲染库

