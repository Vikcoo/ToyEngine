# 渲染流程测试说明

当前 `Main.cpp` 实现了完整的 Vulkan 渲染流程，包括：

1. **初始化**：Context、Surface、PhysicalDevice
2. **设备创建**：LogicalDevice、Queues、CommandPool
3. **资源创建**：SwapChain、RenderPass、Framebuffer、ImageView
4. **管线创建**：GraphicsPipeline（使用着色器）
5. **渲染循环**：完整的帧渲染流程

## 📋 运行前准备

### 1. 编译着色器

在运行程序前，需要先编译着色器文件为 SPIR-V 格式：

```bash
# 使用 Vulkan SDK 的 glslc 编译器
glslc Content/Shaders/Common/triangle.vert -o Content/Shaders/Common/triangle.vert.spv
glslc Content/Shaders/Common/triangle.frag -o Content/Shaders/Common/triangle.frag.spv
```

**注意**：确保 `glslc` 在 PATH 中，或者使用完整路径：
```bash
$VULKAN_SDK/Bin/glslc.exe Content/Shaders/Common/triangle.vert -o Content/Shaders/Common/triangle.vert.spv
$VULKAN_SDK/Bin/glslc.exe Content/Shaders/Common/triangle.frag -o Content/Shaders/Common/triangle.frag.spv
```

### 2. 确保着色器文件存在

编译后应存在以下文件：
- `Content/Shaders/Common/triangle.vert.spv`
- `Content/Shaders/Common/triangle.frag.spv`

## 🚀 运行

运行程序后，你应该看到一个彩色三角形（红、绿、蓝）在窗口中渲染。

## 📝 渲染流程说明

### 每帧执行步骤：

1. **等待上一帧完成**：使用 Fence 确保上一帧渲染完成
2. **获取交换链图像**：从 SwapChain 获取可用的图像索引
3. **录制命令缓冲**：
   - 开始 RenderPass（自动处理布局转换）
   - 绑定 Graphics Pipeline
   - 设置视口和剪裁区域
   - 绘制三角形（3个顶点，使用 `gl_VertexIndex`）
   - 结束 RenderPass
4. **提交命令**：提交到 Graphics Queue，使用 Semaphore 同步
5. **呈现图像**：将渲染结果呈现到窗口
6. **帧同步**：使用 Fence 和 Semaphore 确保正确的执行顺序

### 同步机制

- **Semaphore**：用于 GPU 之间的同步（Acquire → Render → Present）
- **Fence**：用于 CPU-GPU 同步（等待上一帧完成）
- **MAX_FRAMES_IN_FLIGHT = 2**：允许最多 2 帧同时渲染，提高性能

## 🔧 故障排除

### 如果程序无法找到着色器文件：

1. 检查着色器路径是否正确（相对于可执行文件的工作目录）
2. 确保 `.spv` 文件已正确编译
3. 检查日志输出中的错误信息
4. 尝试使用绝对路径

### 如果渲染失败：

1. 检查 Vulkan 验证层输出（如果启用）
2. 确保所有 Vulkan 对象正确创建
3. 检查设备是否支持所需的特性
4. 查看日志中的详细错误信息

### 如果窗口显示黑屏：

1. 检查命令缓冲是否正确录制
2. 检查 Pipeline 是否正确创建
3. 检查 RenderPass 和 Framebuffer 是否匹配
4. 确保着色器代码正确（检查编译错误）

## 📊 性能提示

- 使用 `MAX_FRAMES_IN_FLIGHT` 控制并发帧数
- 避免在渲染循环中创建/销毁 Vulkan 对象
- 使用命令池重用命令缓冲
- 合理使用同步对象，避免过度等待



