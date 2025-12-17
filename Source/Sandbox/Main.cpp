// ToyEngine Sandbox - 完整渲染流程测试
#include "Log/Log.h"
#include "Window.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanSurface.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandPool.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanImageView.h"
#include "VulkanPipeline.h"
#include "VulkanCommandBuffer.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <fstream>

int main()
{
    // 初始化日志系统
    TE::Log::Init();
    
    TE_LOG_INFO("=== ToyEngine Sandbox - Full Rendering Pipeline ===");


    // ============================================================
    // 层次1：初始化
    // ============================================================

    // 1. 创建窗口
    const TE::WindowConfig config{"ToyEngine - Layer 2 Test", 1280, 720, true};

    const auto window = TE::Window::Create(config);
    if (!window) {
        TE_LOG_ERROR("Failed to create window");
        return -1;
    }

    auto context = TE::VulkanContext::Create();
    if (!context) {
        TE_LOG_ERROR("Failed to create Vulkan Context");
        return -1;
    }

    auto surface = context->CreateSurface(*window);
    if (!surface) {
        TE_LOG_ERROR("Failed to create Surface");
        return -1;
    }

    auto devices = context->EnumeratePhysicalDevices();
    if (devices.empty()) {
        TE_LOG_ERROR("No physical devices found");
        return -1;
    }
    auto bestDevice = *std::max_element(devices.begin(), devices.end(),
        [](const auto& a, const auto& b) {
            return a->CalculateScore() < b->CalculateScore();
        });
    bestDevice->PrintInfo();

    auto queueFamilies = bestDevice->FindQueueFamilies(surface.get());
    if (!queueFamilies.IsComplete()) {
        TE_LOG_ERROR("Required queue families not found");
        return -1;
    }
    TE_LOG_INFO("  Graphics Queue Family: {}", queueFamilies.graphics.value());
    TE_LOG_INFO("  Present Queue Family: {}", queueFamilies.present.value());
    if (queueFamilies.compute.has_value()) {
        TE_LOG_INFO("  Compute Queue Family: {}", queueFamilies.compute.value());
    }
    if (queueFamilies.transfer.has_value()) {
        TE_LOG_INFO("  Transfer Queue Family: {}", queueFamilies.transfer.value());
    }

    TE::DeviceConfig deviceConfig;
    deviceConfig.enabledFeatures.samplerAnisotropy = VK_TRUE;  // 启用各向异性过滤
    auto device = TE::VulkanDevice::Create(bestDevice, queueFamilies, deviceConfig);
    if (!device) {
        TE_LOG_ERROR("Failed to create logical device");
        return -1;
    }

    auto graphicsQueue = device->GetGraphicsQueue();
    auto presentQueue = device->GetPresentQueue();

    if (!graphicsQueue) {
        TE_LOG_ERROR("Graphics queue is null");
        return -1;
    }
    if (!presentQueue) {
        TE_LOG_ERROR("Present queue is null");
        return -1;
    }
    TE_LOG_INFO("  Graphics Queue: Family={}, Index={}",
                graphicsQueue->GetFamilyIndex(),
                graphicsQueue->GetQueueIndex());
    TE_LOG_INFO("  Present Queue: Family={}, Index={}",
                presentQueue->GetFamilyIndex(),
                presentQueue->GetQueueIndex());


    while (!window->ShouldClose())
    {
        window->PollEvents();
    }

    // {
    //
    //     // 8. 创建命令池
    //     TE_LOG_INFO("[8] Creating command pool...");
    //     auto commandPool = device->CreateCommandPool(
    //         queueFamilies.graphics.value(),
    //         vk::CommandPoolCreateFlagBits::eResetCommandBuffer
    //     );
    //     if (!commandPool) {
    //         TE_LOG_ERROR("Failed to create command pool");
    //         return -1;
    //     }
    //     TE_LOG_INFO(" Command pool created: QueueFamily={}",
    //                 commandPool->GetQueueFamilyIndex());
    //
    //     // 9. 创建交换链（需要在分配命令缓冲区之前，因为需要知道图像数量）
    //     TE_LOG_INFO("[9] Creating swap chain...");
    //     TE::SwapChainConfig swapChainConfig;
    //     swapChainConfig.preferredFormat = vk::Format::eB8G8R8A8Srgb;
    //     swapChainConfig.preferredPresentMode = vk::PresentModeKHR::eMailbox;
    //     swapChainConfig.imageCount = 3;
    //
    //     auto swapChain = device->CreateSwapChain(
    //         surface,  // 现在接受shared_ptr，直接传递
    //         swapChainConfig,
    //         window->GetWidth(),
    //         window->GetHeight()
    //     );
    //     if (!swapChain) {
    //         TE_LOG_ERROR("Failed to create swap chain");
    //         return -1;
    //     }
    //     TE_LOG_INFO(" Swap chain created:");
    //     TE_LOG_INFO("  Format: {}", vk::to_string(swapChain->GetFormat()));
    //     TE_LOG_INFO("  Extent: {}x{}", swapChain->GetExtent().width, swapChain->GetExtent().height);
    //     TE_LOG_INFO("  Image Count: {}", swapChain->GetImageCount());
    //
    //     // 10. 分配命令缓冲（根据交换链图像数量分配）
    //     TE_LOG_INFO("[10] Allocating command buffers...");
    //     const uint32_t commandBufferCount = swapChain->GetImageCount();
    //     auto commandBuffers = commandPool->AllocateCommandBuffers(commandBufferCount);
    //     TE_LOG_INFO(" Allocated {} command buffer(s)", commandBuffers.size());
    //
    //     // 确保有足够的命令缓冲
    //     if (commandBuffers.size() < commandBufferCount) {
    //         TE_LOG_ERROR("Failed to allocate enough command buffers");
    //         return -1;
    //     }
    //
    //     // ============================================================
    //     // 测试队列操作
    //     // ============================================================
    //
    //     TE_LOG_INFO("[11] Testing queue operations...");
    //
    //     // 测试 WaitIdle
    //     TE_LOG_INFO("  Testing WaitIdle...");
    //     graphicsQueue->WaitIdle();
    //     TE_LOG_INFO("   Queue WaitIdle successful");
    //
    //     // 测试设备 WaitIdle
    //     TE_LOG_INFO("  Testing device WaitIdle...");
    //     device->WaitIdle();
    //     TE_LOG_INFO("   Device WaitIdle successful");
    //
    //     // 测试命令池 Reset
    //     TE_LOG_INFO("  Testing command pool reset...");
    //     commandPool->Reset();
    //     TE_LOG_INFO("   Command pool reset successful");
    //
    //     // ============================================================
    //     // 层次3：资源对象 ✨ 新增测试
    //     // ============================================================
    //
    //     // 11. 创建 RenderPass
    //     TE_LOG_INFO("[11] Creating render pass...");
    //     std::vector<TE::AttachmentConfig> attachments;
    //     attachments.push_back({
    //         swapChain->GetFormat(),
    //         vk::SampleCountFlagBits::e1,
    //         vk::AttachmentLoadOp::eClear,
    //         vk::AttachmentStoreOp::eStore,
    //         vk::ImageLayout::eUndefined,
    //         vk::ImageLayout::ePresentSrcKHR
    //     });
    //
    //     auto renderPass = device->CreateRenderPass(attachments);
    //     if (!renderPass) {
    //         TE_LOG_ERROR("Failed to create render pass");
    //         return -1;
    //     }
    //     TE_LOG_INFO("✓ Render pass created with {} attachment(s)", attachments.size());
    //
    //     // 12. 为交换链图像创建 ImageView 和 Framebuffer
    //     TE_LOG_INFO("[12] Creating image views and framebuffers...");
    //     std::vector<std::unique_ptr<TE::VulkanImageView>> imageViews;
    //     std::vector<std::unique_ptr<TE::VulkanFramebuffer>> framebuffers;
    //
    //     for (uint32_t i = 0; i < swapChain->GetImageCount(); ++i) {
    //         // 创建 ImageView
    //         auto imageView = swapChain->CreateImageView(i);
    //         if (!imageView) {
    //             TE_LOG_ERROR("Failed to create image view {}", i);
    //             return -1;
    //         }
    //         imageViews.push_back(std::move(imageView));
    //
    //         // 创建 Framebuffer
    //         std::vector<vk::ImageView> fbAttachments = {*imageViews[i]->GetHandle()};
    //         auto framebuffer = device->CreateFramebuffer(
    //             *renderPass,
    //             fbAttachments,
    //             swapChain->GetExtent()
    //         );
    //         if (!framebuffer) {
    //             TE_LOG_ERROR("Failed to create framebuffer {}", i);
    //             return -1;
    //         }
    //         framebuffers.push_back(std::move(framebuffer));
    //     }
    //     TE_LOG_INFO(" Created {} image view(s) and framebuffer(s)", imageViews.size());
    //
    //     // ============================================================
    //     // 层次4：命令和管线 ✨ 新增测试
    //     // ============================================================
    //
    //     // 13. 创建图形管线
    //     TE_LOG_INFO("[13] Creating graphics pipeline...");
    //
    //     // 配置管线
    //     TE::GraphicsPipelineConfig pipelineConfig;
    //
    //     // 尝试多个可能的路径
    //     std::string vertexShaderPath = "Content/Shaders/Common/triangle.vert.spv";
    //     std::string fragmentShaderPath = "Content/Shaders/Common/triangle.frag.spv";
    //
    //     // 如果相对路径不存在，尝试使用项目根目录
    //     #ifdef TE_PROJECT_ROOT_DIR
    //     std::string projectRoot = TE_PROJECT_ROOT_DIR;
    //     if (projectRoot.back() != '/' && projectRoot.back() != '\\') {
    //         projectRoot += "/";
    //     }
    //     std::string absVertexPath = projectRoot + vertexShaderPath;
    //     std::string absFragmentPath = projectRoot + fragmentShaderPath;
    //
    //     // 检查文件是否存在
    //     std::ifstream testFile(absVertexPath);
    //     if (testFile.good()) {
    //         vertexShaderPath = absVertexPath;
    //         fragmentShaderPath = absFragmentPath;
    //         TE_LOG_INFO("Using absolute shader paths from project root");
    //     } else {
    //         TE_LOG_INFO("Using relative shader paths (current working directory)");
    //     }
    //     testFile.close();
    //     #endif
    //
    //     pipelineConfig.vertexShaderPath = vertexShaderPath;
    //     pipelineConfig.fragmentShaderPath = fragmentShaderPath;
    //
    //     // 视口和剪裁
    //     pipelineConfig.viewport = vk::Viewport(
    //         0.0f, 0.0f,
    //         static_cast<float>(swapChain->GetExtent().width),
    //         static_cast<float>(swapChain->GetExtent().height),
    //         0.0f, 1.0f
    //     );
    //     pipelineConfig.scissor = vk::Rect2D({0, 0}, swapChain->GetExtent());
    //
    //     // 这个着色器不需要顶点输入（使用 gl_VertexIndex）
    //     // pipelineConfig.vertexBindings = {};
    //     // pipelineConfig.vertexAttributes = {};
    //
    //     auto pipeline = device->CreateGraphicsPipeline(*renderPass, pipelineConfig);
    //     if (!pipeline) {
    //         TE_LOG_ERROR("Failed to create graphics pipeline");
    //         return -1;
    //     }
    //     TE_LOG_INFO(" Graphics pipeline created");
    //     TE_LOG_INFO("  Viewport: {}x{}", swapChain->GetExtent().width, swapChain->GetExtent().height);
    //     TE_LOG_INFO("  Shaders: {} + {}", pipelineConfig.vertexShaderPath, pipelineConfig.fragmentShaderPath);
    //
    //     // ============================================================
    //     // 完整渲染循环 ✨
    //     // ============================================================
    //
    //     TE_LOG_INFO("[14] Starting rendering loop... (Close window to exit)");
    //
    //     // 用于限制并发帧数
    //     constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    //
    //     // 用于标记交换链需要重建的标志（窗口大小改变时设置）
    //     std::atomic<bool> framebufferResized{false};
    //
    //     // 用于标记窗口最小化状态（优雅的窗口状态管理）
    //     std::atomic<bool> windowMinimized{false};
    //
    //     // 为每个并发帧创建信号量（使用 MAX_FRAMES_IN_FLIGHT 模式，避免信号量重用）
    //     // 注意：需要在 lambda 定义前声明，因为 lambda 会使用这些变量
    //     std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    //     std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    //     std::vector<vk::raii::Fence> inFlightFences;
    //
    //     for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    //         imageAvailableSemaphores.push_back(device->CreateSemaphore());
    //         renderFinishedSemaphores.push_back(device->CreateSemaphore());
    //         inFlightFences.push_back(device->CreateFence(true));  // 初始为已信号状态
    //     }
    //
    //     // 跟踪每个交换链图像当前使用的围栏（用于避免信号量重用）
    //     const uint32_t swapChainImageCount = swapChain->GetImageCount();
    //     std::vector<vk::raii::Fence*> imagesInFlight(swapChainImageCount, nullptr);
    //
    //     // 设置窗口大小改变回调（事件驱动，最优雅的方式）
    //     window->SetResizeCallback([&framebufferResized, &windowMinimized](uint32_t width, uint32_t height) {
    //         // 窗口大小改变时，标记需要重建交换链
    //         framebufferResized = true;
    //         // 更新窗口最小化状态（当窗口尺寸为0时，通常表示最小化）
    //         windowMinimized = (width == 0 || height == 0);
    //         TE_LOG_DEBUG("Framebuffer resized: {}x{} (minimized: {})", width, height, windowMinimized.load());
    //     });
    //
    //     // 设置窗口焦点回调（辅助检测窗口状态）
    //     window->SetFocusCallback([&windowMinimized](bool focused) {
    //         if (focused) {
    //             // 窗口获得焦点，可能从最小化恢复
    //             // 注意：不立即设置windowMinimized=false，让图标化回调或渲染循环根据实际情况判断
    //             TE_LOG_DEBUG("Window focused");
    //         } else {
    //             // 窗口失去焦点（但不一定是最小化）
    //             TE_LOG_DEBUG("Window unfocused");
    //         }
    //     });
    //
    //     // 设置窗口图标化回调（正确处理最小化/恢复，这是最准确的方式）
    //     window->SetIconifyCallback([&windowMinimized](bool iconified) {
    //         windowMinimized = iconified;
    //         if (iconified) {
    //             TE_LOG_DEBUG("Window minimized (iconified)");
    //         } else {
    //             TE_LOG_DEBUG("Window restored from minimized (uniconified)");
    //         }
    //     });
    //
    //     // 封装交换链重建逻辑（优雅的函数封装，避免代码重复）
    //     // 注意：使用 [&] 捕获所有引用，包括上面声明的变量
    //     auto RecreateSwapChain = [&]() -> bool {
    //         // 检查窗口是否最小化
    //         if (window->GetWidth() == 0 || window->GetHeight() == 0) {
    //             windowMinimized = true;  // 更新状态
    //             TE_LOG_DEBUG("Window minimized, skipping swap chain recreation");
    //             return false;  // 窗口最小化，不重建
    //         }
    //
    //         TE_LOG_WARN("Recreating swap chain...");
    //         device->WaitIdle();
    //
    //         // 清理旧交换链
    //         {
    //             auto oldSwapChain = std::move(swapChain);
    //             // oldSwapChain 在这里会被销毁
    //         }
    //
    //         // 创建新交换链
    //         swapChain = device->CreateSwapChain(
    //             surface,
    //             swapChainConfig,
    //             window->GetWidth(),
    //             window->GetHeight()
    //         );
    //
    //         if (!swapChain) {
    //             TE_LOG_ERROR("Failed to recreate swap chain");
    //             return false;
    //         }
    //
    //         // 重新创建 ImageView 和 Framebuffer
    //         imageViews.clear();
    //         framebuffers.clear();
    //         for (uint32_t i = 0; i < swapChain->GetImageCount(); ++i) {
    //             auto imageView = swapChain->CreateImageView(i);
    //             if (!imageView) {
    //                 TE_LOG_ERROR("Failed to create image view {}", i);
    //                 return false;
    //             }
    //             imageViews.push_back(std::move(imageView));
    //
    //             std::vector<vk::ImageView> fbAttachments = {*imageViews[i]->GetHandle()};
    //             auto framebuffer = device->CreateFramebuffer(
    //                 *renderPass,
    //                 fbAttachments,
    //                 swapChain->GetExtent()
    //             );
    //             if (!framebuffer) {
    //                 TE_LOG_ERROR("Failed to create framebuffer {}", i);
    //                 return false;
    //             }
    //             framebuffers.push_back(std::move(framebuffer));
    //         }
    //
    //         // 重新分配命令缓冲区
    //         commandBuffers.clear();
    //         const uint32_t newCommandBufferCount = swapChain->GetImageCount();
    //         commandBuffers = commandPool->AllocateCommandBuffers(newCommandBufferCount);
    //         if (commandBuffers.size() < newCommandBufferCount) {
    //             TE_LOG_ERROR("Failed to allocate enough command buffers after swap chain recreation");
    //             return false;
    //         }
    //         TE_LOG_INFO("Reallocated {} command buffer(s)", commandBuffers.size());
    //
    //         // 重新创建信号量
    //         imageAvailableSemaphores.clear();
    //         renderFinishedSemaphores.clear();
    //         for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    //             imageAvailableSemaphores.push_back(device->CreateSemaphore());
    //             renderFinishedSemaphores.push_back(device->CreateSemaphore());
    //         }
    //
    //         // 重置 imagesInFlight 数组
    //         const uint32_t newSwapChainImageCount = swapChain->GetImageCount();
    //         imagesInFlight.clear();
    //         imagesInFlight.resize(newSwapChainImageCount, nullptr);
    //
    //         // 更新视口和剪裁
    //         pipelineConfig.viewport = vk::Viewport(
    //             0.0f, 0.0f,
    //             static_cast<float>(swapChain->GetExtent().width),
    //             static_cast<float>(swapChain->GetExtent().height),
    //             0.0f, 1.0f
    //         );
    //         pipelineConfig.scissor = vk::Rect2D({0, 0}, swapChain->GetExtent());
    //
    //         framebufferResized = false;  // 清除标志
    //         windowMinimized = false;      // 窗口已恢复，更新状态
    //         TE_LOG_INFO("Swap chain recreated successfully");
    //         return true;
    //     };
    //
    //     uint32_t currentFrame = 0;
    //     uint32_t frameCount = 0;
    //
    //     // 清除颜色（深灰色背景，这样三角形会更明显）
    //     vk::ClearValue clearColor;
    //     clearColor.setColor(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));  // 改为黑色背景，三角形会更明显
    //
    //     while (!window->ShouldClose())
    //     {
    //         window->PollEvents();
    //
    //         // 等待上一帧完成（限制并发帧数）
    //         auto waitResult = device->GetHandle().waitForFences(*inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    //         if (waitResult != vk::Result::eSuccess) {
    //             TE_LOG_ERROR("Failed to wait for fence: {}", vk::to_string(waitResult));
    //             break;
    //         }
    //         device->GetHandle().resetFences(*inFlightFences[currentFrame]);
    //
    //         // 检查是否需要重建交换链（窗口大小改变，事件驱动）
    //         if (framebufferResized) {
    //             if (!RecreateSwapChain()) {
    //                 // 重建失败或窗口最小化，跳过这一帧
    //                 continue;
    //             }
    //         }
    //
    //         // ✅ 使用图标化回调检测窗口状态（最准确的方式）
    //         // 如果窗口最小化，跳过渲染
    //         if (windowMinimized) {
    //             // 双重检查：如果窗口尺寸为0，确保状态一致
    //             if (window->GetWidth() == 0 || window->GetHeight() == 0) {
    //                 continue;
    //             }
    //             // 如果窗口尺寸已恢复但状态仍为最小化，可能是图标化回调还未触发
    //             // 尝试重建交换链（如果窗口真的恢复了，重建会成功）
    //             TE_LOG_DEBUG("Window state indicates minimized but size > 0, attempting swap chain recreation");
    //             if (RecreateSwapChain()) {
    //                 // 重建成功，说明窗口已恢复（RecreateSwapChain内部已设置windowMinimized=false）
    //             } else {
    //                 // 重建失败，窗口可能仍处于异常状态，跳过这一帧
    //                 continue;
    //             }
    //         }
    //
    //         // 额外安全检查：如果窗口尺寸为0，跳过渲染
    //         if (window->GetWidth() == 0 || window->GetHeight() == 0) {
    //             windowMinimized = true;  // 同步状态
    //             continue;
    //         }
    //
    //         // 获取交换链图像
    //         // 使用 currentFrame 的信号量获取图像（MAX_FRAMES_IN_FLIGHT 模式）
    //         auto acquireResult = swapChain->AcquireNextImage(
    //             *imageAvailableSemaphores[currentFrame],
    //             nullptr
    //         );
    //         uint32_t imageIndex = acquireResult.imageIndex;
    //         vk::Result result = acquireResult.result;
    //
    //         // 如果该图像正在使用，等待它完成（避免信号量重用问题）
    //         if (imagesInFlight[imageIndex] != nullptr) {
    //             auto imageWaitResult = device->GetHandle().waitForFences(**imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    //             if (imageWaitResult != vk::Result::eSuccess) {
    //                 TE_LOG_ERROR("Failed to wait for image fence: {}", vk::to_string(imageWaitResult));
    //             }
    //         }
    //
    //         // 处理 AcquireNextImage 结果
    //         if (result == vk::Result::eErrorOutOfDateKHR) {
    //             // 交换链过时，重建（优雅的统一处理）
    //             if (!RecreateSwapChain()) {
    //                 // 窗口最小化，跳过这一帧
    //                 continue;
    //             }
    //             // 重建成功，重新获取图像
    //             continue;
    //         } else if (result == vk::Result::eSuboptimalKHR) {
    //             // 交换链不是最优的，但可以继续使用
    //             TE_LOG_DEBUG("Swap chain is suboptimal");
    //         } else if (result != vk::Result::eSuccess) {
    //             TE_LOG_ERROR("Failed to acquire swap chain image: {}", vk::to_string(result));
    //             break;
    //         }
    //
    //         // 重置命令缓冲（确保干净的状态）
    //         // 注意：由于使用了 eResetCommandBuffer 标志，每次录制前应该重置
    //         // 但实际上，由于我们使用了围栏同步，可能不需要每次都重置
    //
    //         // 录制命令缓冲（使用 RAII Scope）
    //         {
    //             auto recording = commandBuffers[imageIndex]->BeginRecording(
    //                 vk::CommandBufferUsageFlagBits::eOneTimeSubmit  // 明确指定这是一次性提交
    //             );
    //             {
    //                 auto renderPassScope = commandBuffers[imageIndex]->BeginRenderPass(
    //                     *renderPass,
    //                     *framebuffers[imageIndex],
    //                     {clearColor}
    //                 );
    //
    //                 // 绑定管线
    //                 commandBuffers[imageIndex]->BindPipeline(*pipeline);
    //
    //                 // 设置视口和剪裁（动态状态）
    //                 // 注意：由于使用了动态状态，这里设置的视口会覆盖管线创建时的设置
    //                 // Vulkan的Y轴默认从底部开始，我们需要翻转Y轴（通过设置负高度）来匹配OpenGL风格
    //                 vk::Viewport viewport;
    //                 viewport.x = 0.0f;
    //                 viewport.y = static_cast<float>(swapChain->GetExtent().height);  // Y坐标从底部开始
    //                 viewport.width = static_cast<float>(swapChain->GetExtent().width);
    //                 viewport.height = -static_cast<float>(swapChain->GetExtent().height);  // 负高度翻转Y轴
    //                 viewport.minDepth = 0.0f;
    //                 viewport.maxDepth = 1.0f;
    //
    //                 // 翻转Y轴使坐标系统从顶部开始（OpenGL风格）
    //                 commandBuffers[imageIndex]->SetViewport(viewport);
    //
    //                 // 确保剪裁区域与交换链匹配
    //                 vk::Rect2D scissor;
    //                 scissor.offset = vk::Offset2D{0, 0};
    //                 scissor.extent = swapChain->GetExtent();
    //                 commandBuffers[imageIndex]->SetScissor(scissor);
    //
    //                 // 绘制三角形（3个顶点，使用 gl_VertexIndex）
    //                 commandBuffers[imageIndex]->Draw(3, 1, 0, 0);
    //
    //             }  // 自动 endRenderPass
    //         }  // 自动 end
    //
    //         // 提交命令缓冲
    //         // 使用 currentFrame 索引所有同步原语（MAX_FRAMES_IN_FLIGHT 模式）
    //         std::vector<vk::CommandBuffer> submitBuffers = {
    //             commandBuffers[imageIndex]->GetRawHandle()
    //         };
    //         std::vector<vk::Semaphore> waitSemaphores = {
    //             *imageAvailableSemaphores[currentFrame]  // 等待获取图像完成
    //         };
    //         std::vector<vk::PipelineStageFlags> waitStages = {
    //             vk::PipelineStageFlagBits::eColorAttachmentOutput
    //         };
    //         std::vector<vk::Semaphore> signalSemaphores = {
    //             *renderFinishedSemaphores[currentFrame]  // 使用当前帧的信号量
    //         };
    //
    //         graphicsQueue->Submit(
    //             submitBuffers,
    //             waitSemaphores,
    //             waitStages,
    //             signalSemaphores,
    //             *inFlightFences[currentFrame]  // 使用当前帧的围栏
    //         );
    //
    //         // 呈现图像
    //         vk::Result presentResult = swapChain->Present(
    //             imageIndex,
    //             *presentQueue,
    //             {*renderFinishedSemaphores[currentFrame]}  // 使用当前帧的信号量
    //         );
    //
    //         // 记录该图像正在使用（用于下一帧检查）
    //         imagesInFlight[imageIndex] = &inFlightFences[currentFrame];
    //
    //         if (presentResult == vk::Result::eErrorOutOfDateKHR) {
    //             // Present 返回 ErrorOutOfDateKHR，标记需要重建（在下一帧处理，事件驱动）
    //             framebufferResized = true;
    //             TE_LOG_DEBUG("Swap chain out of date during present (will recreate next frame)");
    //         } else if (presentResult == vk::Result::eSuboptimalKHR) {
    //             // 交换链不是最优的，但可以继续使用
    //             TE_LOG_DEBUG("Swap chain is suboptimal");
    //         } else if (presentResult != vk::Result::eSuccess) {
    //             TE_LOG_ERROR("Failed to present swap chain image: {}", vk::to_string(presentResult));
    //         }
    //
    //         // 移动到下一帧
    //         currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    //         frameCount++;
    //
    //         // 每60帧输出一次调试信息
    //         if (frameCount % 60 == 0) {
    //             TE_LOG_DEBUG("Frame: {}, ImageIndex: {}, CurrentFrame: {}",
    //                         frameCount, imageIndex, currentFrame);
    //         }
    //
    //         // 第一帧输出详细信息
    //         if (frameCount == 1) {
    //             TE_LOG_INFO("First frame rendered successfully");
    //             TE_LOG_INFO("  SwapChain Extent: {}x{}",
    //                        swapChain->GetExtent().width,
    //                        swapChain->GetExtent().height);
    //             TE_LOG_INFO("  Viewport: {}x{} (Y-flipped)",
    //                        swapChain->GetExtent().width,
    //                        swapChain->GetExtent().height);
    //         }
    //     }
    //
    //     // 等待所有帧完成
    //     device->WaitIdle();
    //
    //     TE_LOG_INFO("[15] Shutting down...");
    //
    //     // 等待设备空闲（确保所有操作完成）
    //     device->WaitIdle();
    //
    //     // 所有资源会自动销毁（RAII）
    //     // 销毁顺序：commandBuffers → commandPool → device → queues → ...
    //
    //     TE_LOG_INFO(" Cleanup complete");
    //
    //     TE_LOG_INFO("=== Test completed successfully ===");
    // }


    return 0;
}