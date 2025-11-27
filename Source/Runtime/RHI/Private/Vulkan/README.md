# Vulkan 封装设计文档

## 概述

本文档描述 ToyEngine 的 Vulkan 封装架构设计。采用分层设计，从底层到高层逐步封装 Vulkan API。

## 设计理念

### 核心原则

1. **RAII（资源获取即初始化）** - 所有资源通过构造获取，析构释放
2. **明确的所有权语义** - 使用智能指针明确资源所有权
3. **类型安全** - 使用 `vk::` C++ 类型而非 C 类型
4. **Passkey 模式** - 防止外部直接构造，强制使用工厂方法
5. **最小化依赖** - 子对象持有父对象的 `weak_ptr`

### 智能指针策略

| 场景 | 使用方式 | 说明 |
|------|---------|------|
| 根对象 | `shared_ptr<T>` | Context 可被多个对象共享 |
| 共享资源 | `shared_ptr<T>` | PhysicalDevice 可被 Device 持有 |
| 独占资源 | `unique_ptr<T>` | Surface 独占所有权 |
| 子引用父 | `weak_ptr<Parent>` | 避免循环引用，不延长生命周期 |
| 临时借用 | `T&` 或 `const T&` | 函数参数，不持有所有权 |

## 架构层次

```
┌─────────────────────────────────────────────┐
│  Application Layer (用户代码)               │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  Layer 4: Command & Pipeline (命令和管线)   │
│  - VulkanCommandBuffer                      │
│  - VulkanPipeline                           │
│  - VulkanShader                             │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  Layer 3: Resources (资源对象) ✅ 已实现     │
│  - VulkanSwapChain                          │
│  - VulkanRenderPass                         │
│  - VulkanFramebuffer                        │
│  - VulkanImageView                          │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  Layer 2: Device & Queue (逻辑设备) ✅ 已实现│
│  - VulkanDevice                             │
│  - VulkanQueue                              │
│  - VulkanCommandPool                        │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  Layer 1: Context (初始化) ✅ 已实现         │
│  - VulkanContext                            │
│  - VulkanPhysicalDevice                     │
│  - VulkanSurface                            │
└─────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────┐
│  Vulkan-Hpp RAII                            │
└─────────────────────────────────────────────┘
```

---

## 层次1：初始化和硬件抽象 ✅

**状态：已实现**

## 核心类

### VulkanContext

管理 Vulkan Instance 和 Debug Messenger。

**创建：**
```cpp
auto context = VulkanContext::Create();
```

**配置：**
```cpp
VulkanContextConfig config;
config.appName = "MyApp";
config.enableValidation = true;  // Debug 模式启用
auto context = VulkanContext::Create(config);
```

**获取 Instance：**
```cpp
const auto& instance = context->GetInstance();
```

## 设计特点

- **RAII**：资源自动管理
- **智能指针**：使用 `shared_ptr` 管理生命周期
- **工厂模式**：通过 `Create()` 创建
- **验证层**：Debug 模式自动启用

### VulkanPhysicalDevice

查询物理设备信息，找到合适的队列族。

**枚举设备：**
```cpp
auto devices = context->EnumeratePhysicalDevices();
```

**选择最佳设备：**
```cpp
auto bestDevice = *std::max_element(devices.begin(), devices.end(),
    [](const auto& a, const auto& b) {
        return a->CalculateScore() < b->CalculateScore();
    });
```

### VulkanSurface

管理窗口表面，查询 Surface 能力。

**创建 Surface：**
```cpp
auto surface = context->CreateSurface(*window);
```

## 使用示例

### 层次1 + 层次2 完整初始化

```cpp
#include "VulkanContext.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandPool.h"

int main() {
    TE::Log::Init();
    
    // 1. 创建窗口
    auto window = TE::Window::Create();
    
    // 2. 创建 Context
    auto context = TE::VulkanContext::Create();
    if (!context) return -1;
    
    // 3. 创建 Surface
    auto surface = context->CreateSurface(*window);
    if (!surface) return -1;
    
    // 4. 枚举并选择物理设备
    auto devices = context->EnumeratePhysicalDevices();
    auto bestDevice = *std::max_element(devices.begin(), devices.end(),
        [](const auto& a, const auto& b) {
            return a->CalculateScore() < b->CalculateScore();
        });
    
    // 5. 查找队列族
    auto queueFamilies = bestDevice->FindQueueFamilies(surface.get());
    if (!queueFamilies.IsComplete()) {
        TE_LOG_ERROR("Required queue families not found");
        return -1;
    }
    
    // 6. 创建逻辑设备 ✨ 新增
    TE::DeviceConfig deviceConfig;
    deviceConfig.enabledFeatures.samplerAnisotropy = VK_TRUE;  // 启用各向异性过滤
    
    auto device = TE::VulkanDevice::Create(bestDevice, queueFamilies, deviceConfig);
    if (!device) return -1;
    
    // 7. 获取队列 ✨ 新增
    auto graphicsQueue = device->GetGraphicsQueue();
    auto presentQueue = device->GetPresentQueue();
    
    // 8. 创建命令池 ✨ 新增
    auto commandPool = device->CreateCommandPool(
        queueFamilies.graphics.value(),
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer
    );
    
    // 9. 分配命令缓冲 ✨ 新增
    auto commandBuffers = commandPool->AllocateBuffers(3);
    
    // 10. 创建同步对象 ✨ 新增
    auto imageAvailableSemaphore = device->CreateSemaphore();
    auto renderFinishedSemaphore = device->CreateSemaphore();
    auto inFlightFence = device->CreateFence(true);  // 初始为已信号状态
    
    TE_LOG_INFO("✓ 初始化完成！");
    TE_LOG_INFO("  Device: {}", bestDevice->GetDeviceName());
    TE_LOG_INFO("  Graphics Queue: {}", queueFamilies.graphics.value());
    TE_LOG_INFO("  Command Buffers: {}", commandBuffers.size());
    
    // 等待设备空闲
    device->WaitIdle();
    
    return 0;
}
```

## 设计特点

- **RAII**：所有资源自动管理
- **智能指针**：`shared_ptr`、`unique_ptr`、`weak_ptr` 明确所有权
- **Passkey 模式**：防止外部直接构造
- **缓存机制**：PhysicalDevice 查询结果缓存
- **策略模式**：Surface 自动选择最佳配置

## 注意事项

1. **Vulkan SDK**：需要安装 Vulkan SDK 1.3+
2. **验证层**：Debug 启用，Release 禁用
3. **生命周期**：Context → Surface/PhysicalDevice
4. **队列族**：使用 Surface 查找支持呈现的队列族

---

## 层次2：逻辑设备和队列 ✅

**状态：已实现**

### VulkanDevice

逻辑设备，作为资源工厂。

**职责：**
- 创建逻辑设备
- 管理队列
- 创建资源（SwapChain、Buffer、Image 等）
- 管理 PipelineCache

**设计：**
```cpp
class VulkanDevice : public std::enable_shared_from_this<VulkanDevice> {
public:
    struct PrivateTag { /* ... */ };
    
    // 工厂方法
    [[nodiscard]] static std::shared_ptr<VulkanDevice> Create(
        std::shared_ptr<VulkanPhysicalDevice> physicalDevice,
        const QueueFamilyIndices& queueFamilies,
        const vk::PhysicalDeviceFeatures& features = {}
    );
    
    explicit VulkanDevice(PrivateTag, /* ... */);
    
    // 获取队列
    [[nodiscard]] std::shared_ptr<VulkanQueue> GetGraphicsQueue() const;
    [[nodiscard]] std::shared_ptr<VulkanQueue> GetPresentQueue() const;
    [[nodiscard]] std::shared_ptr<VulkanQueue> GetComputeQueue() const;
    
    // 创建资源
    [[nodiscard]] std::unique_ptr<VulkanSwapChain> CreateSwapChain(/* ... */);
    [[nodiscard]] std::unique_ptr<VulkanCommandPool> CreateCommandPool(/* ... */);
    [[nodiscard]] vk::raii::Semaphore CreateSemaphore();
    [[nodiscard]] vk::raii::Fence CreateFence(bool signaled = true);
    
    // 等待设备空闲
    void WaitIdle();
    
    [[nodiscard]] const vk::raii::Device& GetHandle() const { return m_device; }
    
private:
    std::shared_ptr<VulkanPhysicalDevice> m_physicalDevice;
    vk::raii::Device m_device{nullptr};
    vk::raii::PipelineCache m_pipelineCache{nullptr};
    
    std::vector<std::shared_ptr<VulkanQueue>> m_queues;
};
```

**所有权：**
- `shared_ptr<VulkanDevice>` - 可被多个资源共享
- 持有 `shared_ptr<VulkanPhysicalDevice>`
- 拥有 `unique_ptr<VulkanQueue>`

### VulkanQueue

队列封装，用于提交命令。

**设计：**
```cpp
class VulkanQueue {
public:
    struct PrivateTag { /* ... */ };
    
    explicit VulkanQueue(PrivateTag, 
                        vk::raii::Queue queue,
                        uint32_t familyIndex,
                        uint32_t queueIndex);
    
    // 提交命令
    void Submit(
        const std::vector<vk::CommandBuffer>& cmdBuffers,
        const std::vector<vk::Semaphore>& waitSemaphores = {},
        const std::vector<vk::PipelineStageFlags>& waitStages = {},
        const std::vector<vk::Semaphore>& signalSemaphores = {},
        vk::Fence fence = nullptr
    );
    
    void WaitIdle();
    
    [[nodiscard]] uint32_t GetFamilyIndex() const { return m_familyIndex; }
    [[nodiscard]] const vk::raii::Queue& GetHandle() const { return m_queue; }
    
private:
    vk::raii::Queue m_queue;
    uint32_t m_familyIndex;
    uint32_t m_queueIndex;
};
```

### VulkanCommandPool

命令池，分配命令缓冲。

**设计：**
```cpp
class VulkanCommandPool {
public:
    struct PrivateTag { /* ... */ };
    
    explicit VulkanCommandPool(PrivateTag,
                              std::shared_ptr<VulkanDevice> device,
                              uint32_t queueFamilyIndex,
                              vk::CommandPoolCreateFlags flags = {});
    
    // 分配命令缓冲
    [[nodiscard]] std::vector<std::unique_ptr<VulkanCommandBuffer>> 
        AllocateBuffers(uint32_t count);
    
    void Reset(vk::CommandPoolResetFlags flags = {});
    
    [[nodiscard]] const vk::raii::CommandPool& GetHandle() const { return m_pool; }
    
private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::CommandPool m_pool{nullptr};
    uint32_t m_queueFamilyIndex;
};
```

---

## 层次3：资源对象 ✅

**状态：已实现**

### VulkanSwapChain

交换链管理。

**职责：**
- 创建交换链
- 管理呈现图像
- 获取下一张图像
- 呈现图像

**设计：**
```cpp
struct SwapChainConfig {
    vk::Format preferredFormat = vk::Format::eB8G8R8A8Srgb;
    vk::ColorSpaceKHR colorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eMailbox;
    uint32_t imageCount = 3;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment;
};

class VulkanSwapChain {
public:
    struct PrivateTag { /* ... */ };
    
    explicit VulkanSwapChain(PrivateTag,
                            std::shared_ptr<VulkanDevice> device,
                            VulkanSurface& surface,
                            const SwapChainConfig& config);
    
    // 重建交换链（返回新对象）
    [[nodiscard]] std::unique_ptr<VulkanSwapChain> Recreate() const;
    
    // 获取图像
    struct AcquireResult {
        uint32_t imageIndex;
        vk::Result result;
    };
    [[nodiscard]] AcquireResult AcquireNextImage(
        vk::Semaphore signalSemaphore,
        vk::Fence fence = nullptr
    );
    
    // 呈现
    [[nodiscard]] vk::Result Present(
        uint32_t imageIndex,
        VulkanQueue& presentQueue,
        const std::vector<vk::Semaphore>& waitSemaphores
    );
    
    [[nodiscard]] vk::Format GetFormat() const { return m_format; }
    [[nodiscard]] vk::Extent2D GetExtent() const { return m_extent; }
    [[nodiscard]] uint32_t GetImageCount() const { return m_images.size(); }
    [[nodiscard]] vk::Image GetImage(uint32_t index) const { return m_images[index]; }
    
private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::SwapchainKHR m_swapchain{nullptr};
    std::vector<vk::Image> m_images;  // 不拥有，由 swapchain 管理
    vk::Format m_format;
    vk::Extent2D m_extent;
};
```

### VulkanRenderPass

渲染通道。

**设计：**
```cpp
struct AttachmentConfig {
    vk::Format format;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR;
};

class VulkanRenderPass {
public:
    struct PrivateTag { /* ... */ };
    
    explicit VulkanRenderPass(PrivateTag,
                             std::shared_ptr<VulkanDevice> device,
                             const std::vector<AttachmentConfig>& attachments);
    
    [[nodiscard]] const vk::raii::RenderPass& GetHandle() const { return m_renderPass; }
    
private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::RenderPass m_renderPass{nullptr};
};
```

### VulkanFramebuffer

帧缓冲。

**设计：**
```cpp
class VulkanFramebuffer {
public:
    struct PrivateTag { /* ... */ };
    
    explicit VulkanFramebuffer(PrivateTag,
                              std::shared_ptr<VulkanDevice> device,
                              const VulkanRenderPass& renderPass,
                              const std::vector<vk::ImageView>& attachments,
                              vk::Extent2D extent);
    
    [[nodiscard]] const vk::raii::Framebuffer& GetHandle() const { return m_framebuffer; }
    [[nodiscard]] vk::Extent2D GetExtent() const { return m_extent; }
    
private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::Framebuffer m_framebuffer{nullptr};
    vk::Extent2D m_extent;
};
```

---

## 层次4：命令和管线（规划）

**状态：待实现**

### VulkanCommandBuffer

命令缓冲，使用 RAII Scope 模式。

**设计：**
```cpp
class VulkanCommandBuffer {
public:
    struct PrivateTag { /* ... */ };
    
    explicit VulkanCommandBuffer(PrivateTag,
                                std::shared_ptr<VulkanDevice> device,
                                vk::raii::CommandBuffer buffer);
    
    // RAII 录制 Scope
    class RecordingScope {
    public:
        RecordingScope(VulkanCommandBuffer& cmd, vk::CommandBufferUsageFlags flags = {});
        ~RecordingScope();  // 自动 end
        
        RecordingScope(const RecordingScope&) = delete;
        RecordingScope& operator=(const RecordingScope&) = delete;
        
    private:
        VulkanCommandBuffer& m_cmd;
    };
    
    [[nodiscard]] RecordingScope BeginRecording(vk::CommandBufferUsageFlags flags = {});
    
    // RAII RenderPass Scope
    class RenderPassScope {
    public:
        RenderPassScope(VulkanCommandBuffer& cmd,
                       const VulkanRenderPass& renderPass,
                       const VulkanFramebuffer& framebuffer,
                       const std::vector<vk::ClearValue>& clearValues);
        ~RenderPassScope();  // 自动 endRenderPass
        
        RenderPassScope(const RenderPassScope&) = delete;
        RenderPassScope& operator=(const RenderPassScope&) = delete;
        
    private:
        VulkanCommandBuffer& m_cmd;
    };
    
    [[nodiscard]] RenderPassScope BeginRenderPass(/* ... */);
    
    // 命令录制
    void BindPipeline(const VulkanPipeline& pipeline);
    void SetViewport(const vk::Viewport& viewport);
    void SetScissor(const vk::Rect2D& scissor);
    void Draw(uint32_t vertexCount, uint32_t instanceCount = 1, 
              uint32_t firstVertex = 0, uint32_t firstInstance = 0);
    
    [[nodiscard]] vk::CommandBuffer GetHandle() const { return *m_buffer; }
    
private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::CommandBuffer m_buffer;
};
```

**使用示例：**
```cpp
// RAII 自动配对 begin/end
{
    auto recording = cmdBuffer->BeginRecording();
    {
        auto renderPass = cmdBuffer->BeginRenderPass(renderPass, framebuffer, clearValues);
        
        cmdBuffer->BindPipeline(*pipeline);
        cmdBuffer->SetViewport(viewport);
        cmdBuffer->SetScissor(scissor);
        cmdBuffer->Draw(3);
        
    }  // renderPass 自动 end
}  // recording 自动 end
```

### VulkanPipeline

图形管线。

**设计：**
```cpp
struct GraphicsPipelineConfig {
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
    vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
    std::vector<vk::DynamicState> dynamicStates;
    // ... 其他配置
};

class VulkanPipeline {
public:
    struct PrivateTag { /* ... */ };
    
    explicit VulkanPipeline(PrivateTag,
                           std::shared_ptr<VulkanDevice> device,
                           const VulkanRenderPass& renderPass,
                           const GraphicsPipelineConfig& config);
    
    void Bind(vk::raii::CommandBuffer& cmdBuffer);
    
    [[nodiscard]] const vk::raii::Pipeline& GetHandle() const { return m_pipeline; }
    
private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::Pipeline m_pipeline{nullptr};
    vk::raii::PipelineLayout m_layout{nullptr};
};
```

---

## 生命周期管理

### 对象依赖图

```
Context (shared_ptr) ← 根对象
  ├→ PhysicalDevice (shared_ptr, weak_ptr<Context>)
  │   └→ Device (shared_ptr, shared_ptr<PhysicalDevice>)
  │       ├→ Queue (unique_ptr)
  │       ├→ CommandPool (unique_ptr, shared_ptr<Device>)
  │       │   └→ CommandBuffer (unique_ptr, shared_ptr<Device>)
  │       ├→ SwapChain (unique_ptr, shared_ptr<Device>)
  │       ├→ RenderPass (unique_ptr, shared_ptr<Device>)
  │       └→ Pipeline (unique_ptr, shared_ptr<Device>)
  └→ Surface (unique_ptr, weak_ptr<Context>)
```

### 销毁顺序

由于使用智能指针，销毁顺序自动正确：

1. 高层资源（Pipeline、CommandBuffer）先销毁
2. 中层资源（SwapChain、RenderPass）随后
3. Device 和 Queue 销毁
4. PhysicalDevice 和 Surface 销毁
5. Context 最后销毁

**示例：**
```cpp
{
    auto context = VulkanContext::Create();
    auto devices = context->EnumeratePhysicalDevices();
    auto surface = context->CreateSurface(*window);
    auto device = VulkanDevice::Create(devices[0], queueFamilies);
    auto swapchain = device->CreateSwapChain(*surface, config);
    
    // 离开作用域时按正确顺序自动销毁：
    // swapchain → device → devices → surface → context
}
```

---

## 设计模式

### 1. Passkey 模式

防止外部直接构造，强制使用工厂方法。

```cpp
class VulkanContext {
public:
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanContext;  // 只有 VulkanContext 能构造
    };
    
    // 外部无法直接调用构造函数
    explicit VulkanContext(PrivateTag, const VulkanContextConfig& config);
    
    // 只能通过工厂方法创建
    [[nodiscard]] static std::shared_ptr<VulkanContext> Create(/* ... */) {
        return std::make_shared<VulkanContext>(PrivateTag{}, config);
    }
};
```

### 2. RAII Scope 模式

自动配对 begin/end 操作。

```cpp
class RecordingScope {
public:
    RecordingScope(CommandBuffer& cmd) : m_cmd(cmd) {
        m_cmd.Begin();
    }
    ~RecordingScope() {
        m_cmd.End();
    }
private:
    CommandBuffer& m_cmd;
};

// 使用
{
    auto recording = cmd.BeginRecording();
    // ... 录制命令 ...
}  // 自动调用 End()
```

### 3. 工厂模式

所有资源通过工厂方法创建。

```cpp
// Context 创建 PhysicalDevice
auto devices = context->EnumeratePhysicalDevices();

// Device 创建资源
auto swapchain = device->CreateSwapChain(/* ... */);
auto commandPool = device->CreateCommandPool(/* ... */);
```

### 4. 不可变性 + 重建

资源重建返回新对象，而不是修改现有对象。

```cpp
// 重建 SwapChain
auto newSwapChain = oldSwapChain->Recreate();
// oldSwapChain 自动销毁
```

---

## 错误处理策略

### 1. 工厂方法返回 nullptr

```cpp
auto context = VulkanContext::Create();
if (!context) {
    // 初始化失败
    return;
}
```

### 2. 异常安全

所有 RAII 对象保证异常安全，即使抛出异常也能正确释放资源。

### 3. 日志记录

所有错误通过 `TE_LOG_ERROR` 记录，便于调试。

---

## 性能优化

### 1. 查询结果缓存

PhysicalDevice 的属性查询结果缓存，避免重复调用 Vulkan API。

```cpp
vk::PhysicalDeviceProperties VulkanPhysicalDevice::GetProperties() const {
    if (!m_cachedProperties.has_value()) {
        m_cachedProperties = m_device.getProperties();
    }
    return m_cachedProperties.value();
}
```

### 2. 移动语义

所有资源类支持移动，避免不必要的拷贝。

```cpp
VulkanSwapChain(VulkanSwapChain&&) noexcept = default;
VulkanSwapChain& operator=(VulkanSwapChain&&) noexcept = default;
```

### 3. Pipeline Cache

Device 创建时自动创建 PipelineCache，加速管线创建。

---

## 实现清单

- [x] **层次1：初始化**
  - [x] VulkanContext
  - [x] VulkanPhysicalDevice
  - [x] VulkanSurface
  - [x] VulkanUtils

- [x] **层次2：设备和队列**
  - [x] VulkanDevice
  - [x] VulkanQueue
  - [x] VulkanCommandPool

- [x] **层次3：资源对象**
  - [x] VulkanSwapChain
  - [x] VulkanRenderPass
  - [x] VulkanFramebuffer
  - [x] VulkanImageView

- [ ] **层次4：命令和管线**
  - [ ] VulkanCommandBuffer
  - [ ] VulkanPipeline
  - [ ] VulkanShader
  - [ ] VulkanDescriptorSet

---

## 参考资料

- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [Vulkan-Hpp](https://github.com/KhronosGroup/Vulkan-Hpp)
- [Vulkan Samples](https://github.com/KhronosGroup/Vulkan-Samples)
- Unreal Engine 5 RHI 设计
- The-Forge 渲染库

---

**最后更新：** 2024年11月  
**版本：** 1.0  
**状态：** 层次1已实现，层次2-4待实现

