# Vulkan 交换链与帧同步基础

## 这套机制解决什么问题

窗口系统拥有一组可呈现图像，应用不能像 OpenGL 默认帧缓冲那样假设“当前窗口后缓冲”始终隐式可用。Vulkan 每帧必须显式完成三段协作：从交换链取得一张图像、把 GPU 命令提交到队列、请求窗口系统呈现该图像。CPU、GPU 与 Present Engine 各自异步运行，因此还要用 fence 和 semaphore 表达谁可以继续。

ToyEngine 从阶段 A 建立并由阶段 B 延续的主路径是：

```text
等待 frame fence
  -> Acquire(image-available semaphore, imageIndex)
  -> 必要时等待 imagesInFlight[imageIndex]
  -> 录制 layout transition + Dynamic Rendering clear + transition to Present
  -> Submit(wait image-available, signal render-finished[imageIndex], frame fence)
  -> Present(wait render-finished[imageIndex], imageIndex)
```

## frame index 不等于 image index

frames-in-flight 是应用为 CPU/GPU 并行准备的录制槽位。例如两个 frame slot 允许 CPU 准备下一帧时，GPU 仍执行上一帧。Swapchain image 是窗口系统维护的实际呈现图像，常见数量可能是 3。Acquire 返回哪个 image index 由 WSI 决定，顺序不保证与 frame index 相同。

因此两类资源应分别归属：

| 归属 | 典型对象 | 复用安全条件 |
| --- | --- | --- |
| frame slot | CommandPool、CommandBuffer、acquire semaphore、submit fence | 对应 submit fence 已完成 |
| swapchain image | ImageView、present semaphore、图像当前 layout、image-in-flight fence 映射 | 图像重新 Acquire，且其最近一次 GPU 使用 fence 已完成 |

把两类索引混用，通常不会立即崩溃，却会在交换链图像数、呈现延迟或 Acquire 顺序变化时出现偶发 Validation Error。

## Fence 与 Semaphore 的分工

Fence 用于让 CPU 观察 GPU 提交是否完成。ToyEngine 在复用某个 frame slot 的 CommandPool 前等待该 slot 的 fence；Submit 前 reset fence，Submit 后由 GPU signal。Fence 也被记录到 `imagesInFlight[imageIndex]`，防止另一 frame slot 提前复用仍在 GPU 上的交换链图像。

Semaphore 用于 GPU 队列和 Present Engine 之间排序，不需要 CPU 主动等待：

- Acquire signal `imageAvailable`，Submit 等待它，保证图像已经归应用使用。
- Submit signal `renderFinished[imageIndex]`，Present 等待它，保证渲染和 layout transition 已完成。

二进制 semaphore 再次 signal 前必须确认上一次 wait 已经消费。frame fence 只证明 Graphics Queue 已完成，不能证明 Present Engine 已结束对 render-finished semaphore 的等待。因此 render-finished semaphore 按 swapchain image 分配；相同图像能再次 Acquire 时，才可安全复用它对应的 present semaphore。另一条可选路线是使用 `VK_KHR_swapchain_maintenance1` 的 present fence，但阶段 A 选择更普遍、扩展依赖更少的按图像方案。

## Swapchain 重建不是普通 resize

以下情况都可能要求重建：

- framebuffer 物理像素尺寸改变；
- VSync 偏好改变，需要重新选择 present mode；
- Acquire/Present 返回 `VK_ERROR_OUT_OF_DATE_KHR`；
- 返回 `VK_SUBOPTIMAL_KHR`，当前交换链仍可用但已不是最佳匹配；
- 窗口从最小化恢复。

最小化时 surface extent 可能为零，Vulkan 禁止创建 `0×0` Swapchain。尺寸变化又可能在不同线程/系统回调间异步发生，所以既要在 BeginFrame 入口检查 framebuffer，也要在查询 surface capabilities 后检查最终 extent。零尺寸不是错误，应该返回“本帧跳过”，保留待重建状态并等待恢复。

阶段 A 在重建前使用 `vkDeviceWaitIdle`。这种做法会造成一次全设备停顿，但对象数量少、逻辑直观，适合先建立正确性基线。后续若 resize 卡顿成为问题，可为旧交换链及其资源建立按 fence 回收的退休队列，而不是过早增加生命周期复杂度。

## 图像 Layout 与 Dynamic Rendering

新交换链图像第一次使用时可从 `VK_IMAGE_LAYOUT_UNDEFINED` 转为 `VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL`，因为清屏不需要保留旧内容；后续从 `VK_IMAGE_LAYOUT_PRESENT_SRC_KHR` 转入颜色附件布局。清屏完成后再转回 Present layout。

Dynamic Rendering 用 `vkCmdBeginRendering/vkCmdEndRendering` 直接描述本次附件，不需要预先创建 `VkRenderPass` 和 `VkFramebuffer`。阶段 A 用它完成单颜色附件清屏；阶段 B 加入 Graphics Pipeline 与 D32 深度附件，并要求 pipeline rendering info 与当前颜色/深度格式完全匹配。

## VSync 与 Present Mode

FIFO 是 Vulkan 实现必须支持的模式，会按显示刷新节奏排队，适合作为 VSync 开启路径。关闭 VSync 时可优先选择 MAILBOX；若不可用再尝试 IMMEDIATE，最终仍回退 FIFO。VSync 属于交换链选择，而不是 GLFW OpenGL Context 状态；No-API 窗口不能调用 `glfwSwapInterval()`。

## 排查顺序

出现黑屏、卡死、Validation 或 resize 问题时，优先检查：

1. frame index 与 image index 是否分别管理资源。
2. acquire semaphore 是否只在对应 Submit 中等待，present semaphore 是否可能过早复用。
3. CommandPool reset 前是否已等待 frame fence。
4. 同一交换链图像是否等待过 `imagesInFlight` 记录的 fence。
5. Acquire/Present 的 `OUT_OF_DATE/SUBOPTIMAL` 是否统一进入待重建状态。
6. 最终 surface extent 是否为零，是否错误地把最小化当作致命错误。
7. Dynamic Rendering 的 ImageView、format、extent、layout 与 barrier 是否一致。

项目实现入口位于 `Source/Runtime/RHIVulkan/Private/VulkanDevice.cpp` 与 `VulkanCommandBuffer.cpp`；架构取舍见 [`架构优化复盘-Vulkan后端阶段A`](../architecture/架构优化复盘-Vulkan后端阶段A.md)。
