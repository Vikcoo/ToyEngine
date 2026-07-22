# 架构优化复盘：Vulkan 后端阶段 A

## 为什么先做最小呈现垂直切片

Vulkan 后端若从静态网格、材质和 PBR 一次性起步，Instance、设备选择、交换链、同步、资源内存、Descriptor 与 Pipeline 的错误会同时出现，Validation 信息很难归因。前置阶段已经把帧生命周期和显式资源契约放入 RHI，因此阶段 A 只验证“窗口到 GPU 呈现”的最小闭环：创建设备、Acquire 图像、录制清屏、提交、Present、resize/minimize 恢复和关闭。

阶段 A 不用返回空壳 Buffer/Pipeline 来欺骗现有 Renderer。新增 `RHIBackendTraits::bSupportsSceneRendering` 能力门控：OpenGL 继续执行完整 Sandbox 场景，Vulkan 暂时进入 bootstrap-clear 模式。Engine 仍使用公共 `BeginFrame/EndFrame` 与 `RHIRenderPassBeginInfo`，但跳过场景回调、资源准备和应用帧更新。这使“后端能够呈现”和“后端能够运行完整场景”成为两个明确能力，而不是由调用失败隐式区分。

## 模块与构建边界

`RHIVulkan` 是与 `RHIOpenGL` 平级的静态模块，公开面只有 `CreateRHIVulkanDevice()`；Vulkan 头文件和原生类型留在后端内部。Engine 工厂在配置期选择后端，RHI 抽象层不反向依赖实现。

CMake 要求 `TE_RHI_OPENGL` 与 `TE_RHI_VULKAN` 恰好启用一个。Vulkan 配置通过 `find_package(Vulkan 1.3 REQUIRED)` 获得 Headers 与 Loader，通过 SDK 或 PATH 获得 `glslc`；Validation 默认开启。未实现的 D3D12 选项会明确失败，避免“配置成功但运行时没有 Device”的假状态。

SDK 属于开发机依赖，不复制进源码树。这样避免提交大型、平台相关且由官方安装器维护的二进制；构建目录只保存生成的 SPIR-V。安装后若当前 IDE/终端环境未刷新，配置命令应显式传入 `VULKAN_SDK`、`Vulkan_ROOT` 和 SDK `Bin`。

## Device 与交换链生命周期

初始化顺序为：

1. 检查 GLFW Vulkan 支持和 Validation Layer。
2. 创建 Vulkan 1.3 Instance 与 Debug Messenger。
3. 从 GLFW No-API 窗口创建 Surface。
4. 枚举 PhysicalDevice，要求 Graphics/Present Queue、`VK_KHR_swapchain`、可用 surface format/present mode 和 Vulkan 1.3 Dynamic Rendering。
5. 创建 LogicalDevice 与队列。
6. 创建至少两个 frame slot；Swapchain 延迟到第一帧按真实 framebuffer 尺寸创建。

交换链 format 优先选择 BGRA8/RGBA8 UNorm，再回退到 sRGB 格式；VSync 使用强制可用的 FIFO，关闭 VSync 时优先 MAILBOX、其次 IMMEDIATE。`RHIFormat` 补充 BGRA8 枚举，使呈现目标格式可以通过公共 Pipeline 描述继续传递到阶段 B。

resize、VSync 改变、`VK_ERROR_OUT_OF_DATE_KHR` 和 `VK_SUBOPTIMAL_KHR` 都只设置同一“待重建”状态。阶段 A 为简化生命周期并确保正确性，在交换链重建前调用 `vkDeviceWaitIdle`；阶段 B/C 引入大量资源后可再评估基于 fence retirement 的无全局等待重建。

## 两类索引与同步设计

frame slot index 与 swapchain image index 不能混为一谈。前者表示 CPU 当前可以复用哪组录制资源，后者由 WSI Acquire 决定，数量和轮转顺序都可能不同。

- 每个 frame slot：CommandPool、Primary CommandBuffer、image-available semaphore、submit fence。
- 每个 swapchain image：render-finished semaphore、最近使用该图像的 in-flight fence、是否已经从 `Undefined` 进入过呈现布局的状态。

BeginFrame 先等待当前 frame fence，再 Acquire 图像；若该图像还映射到另一个未完成 fence，则继续等待。命令提交等待当前 frame 的 image-available semaphore，完成后信号当前图像的 render-finished semaphore，Present 等待后者。

render-finished semaphore 必须按交换链图像分配。最初按 frame slot 分配的实现通过普通运行，但 Validation 指出 Present Engine 可能仍在等待该 semaphore，而 CPU 已轮转回同一 frame slot 并再次 signal。只有相同交换链图像重新 Acquire，才足以证明该图像上一次 Present 对应的等待已经结束；因此按 image index 复用才满足二进制 semaphore 生命周期。

## Dynamic Rendering 清屏

阶段 A 不创建 RHI RenderPass 对象或 Graphics Pipeline。BeginFrame 把 Acquire 图像从 `Undefined/Present` 转换为 `COLOR_ATTACHMENT_OPTIMAL`，Engine 录制默认目标的 `BeginRenderPass`，Vulkan CommandBuffer 用 `vkCmdBeginRendering` 执行清除并设置动态 viewport/scissor；EndFrame 再转换到 `PRESENT_SRC_KHR` 后提交。

Vulkan 后端通过 `AdjustProjectionMatrix()` 翻转投影 Y，viewport 保持正高度。两种方式都能适配 Vulkan 的 Y 方向，但不能同时使用，否则阶段 B 的网格会发生双重翻转。清屏本身不使用 Shader，但 Vulkan 构建仍编译全部共享 GLSL 到 SPIR-V，提前验证 set/binding 宏与工具链闭环。

## 最小化与空转处理

仅在 BeginFrame 入口检查 framebuffer 尺寸不够安全：窗口可能在检查之后、等待 Device idle 或查询 surface capabilities 之前异步最小化，最终 `currentExtent` 变成 `0×0`。因此交换链创建还会检查最终 `VkExtent2D`，零尺寸返回 `Skipped`，不调用 `vkCreateSwapchainKHR`，恢复后由待重建状态重新进入创建流程。

最小化时连续返回 `Skipped` 会让单线程主循环无上限空转。Engine 对跳过帧统一睡眠 16 ms；这同样改善 OpenGL 的零尺寸路径，且不会把平台等待 API 泄漏到 RHI。

## Validation 暴露并修复的问题

首次运行发现并修复了两项真实问题：

- 每 frame slot 的 render-finished semaphore 会在 Present 尚未释放时被再次 signal，改为每 swapchain image 一个。
- 最小化竞态可创建 `0×0` Swapchain，并继续录制零宽 Dynamic Rendering；改为在最终 extent 处二次短路，并对 `Skipped` 帧节流。

No-API 窗口还要求 Platform 的 `SetVSync()` 只保存偏好，不调用需要当前 OpenGL Context 的 `glfwSwapInterval()`。Vulkan Device 在下一帧读取偏好并选择 present mode。

## 收益、限制与下一阶段

阶段 A 已建立可独立运行和排障的 Vulkan 基线：后端装配、设备能力筛选、交换链状态机、frame/image 双索引、Validation 日志和窗口恢复都已经真实工作。后续阶段可以把问题限制在资源与绘制层，而无需反复怀疑 WSI 基础设施。

当前限制是：Buffer、Image、Sampler、Staging、ShaderModule、Descriptor、Pipeline、离屏 RenderTarget、transient Uniform 和通用资源延迟释放均未实现；`bSupportsSceneRendering` 因而保持 `false`。阶段 B 应先落地内存分配与基础资源，再完成单静态网格 Forward 垂直切片。Vulkan 完整场景功能尚不与 OpenGL 对等。

## 验证结果（2026-07-22）

- Vulkan SDK/Loader/Validation/驱动检查通过，选择 NVIDIA GeForce RTX 5070 Ti。
- CLion MinGW Release 完整构建通过，7 个 Shader 成功生成 SPIR-V。
- Validation 开启时连续运行、resize、最小化/恢复和正常关闭无 warning/error。
- 最小化期间主循环从百万级空转降到约 33～35 FPS，恢复后回到 FIFO 呈现节奏。
- OpenGL-only Release 重新配置与全量构建通过，Sandbox 完成场景/Pipeline 创建并正常关闭；Vulkan/OpenGL 构建目录中的 `MathTest`、`RHITransientAllocatorTest`、`SimpleTest` 均通过。

后续实现顺序继续以 [`Vulkan后端实施前提与阻塞项处理方案`](./Vulkan后端实施前提与阻塞项处理方案.md) 中的阶段 B～E 为准。
