// ToyEngine Sandbox - 测试层次3（SwapChain、RenderPass、Framebuffer）
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
#include <algorithm>

int main()
{
    // 初始化日志系统
    TE::Log::Init();
    
    TE_LOG_INFO("=== ToyEngine Sandbox - Testing Layer 3 ===");


    // ============================================================
    // 层次1：初始化
    // ============================================================

    // 1. 创建窗口
    TE::WindowConfig config;
    config.title = "ToyEngine - Layer 2 Test";
    config.width = 1280;
    config.height = 720;
    config.resizable = true;

    TE_LOG_INFO("[1] Creating window...");
    const auto window = TE::Window::Create(config);
    if (!window) {
        TE_LOG_ERROR("Failed to create window");
        return -1;
    }

    // 2. 创建 Context
    TE_LOG_INFO("[2] Creating Vulkan Context...");
    auto context = TE::VulkanContext::Create();
    if (!context) {
        TE_LOG_ERROR("Failed to create Vulkan Context");
        return -1;
    }

    // 3. 创建 Surface
    TE_LOG_INFO("[3] Creating Surface...");
    auto surface = context->CreateSurface(*window);
    if (!surface) {
        TE_LOG_ERROR("Failed to create Surface");
        return -1;
    }

    // 4. 枚举并选择物理设备
    TE_LOG_INFO("[4] Enumerating physical devices...");
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

    // 5. 查找队列族
    TE_LOG_INFO("[5] Finding queue families...");
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

    // ============================================================
    // 层次2：设备和队列 ✨ 新增测试
    // ============================================================

    // 6. 创建设备配置
    TE_LOG_INFO("[6] Creating logical device...");
    TE::DeviceConfig deviceConfig;
    deviceConfig.enabledFeatures.samplerAnisotropy = VK_TRUE;  // 启用各向异性过滤

    auto device = TE::VulkanDevice::Create(bestDevice, queueFamilies, deviceConfig);
    if (!device) {
        TE_LOG_ERROR("Failed to create logical device");
        return -1;
    }
    TE_LOG_INFO(" Logical device created");

    // 7. 获取队列
    TE_LOG_INFO("[7] Getting queues...");
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

    // 检查队列是否共享
    if (graphicsQueue == presentQueue) {
        TE_LOG_INFO("   Graphics and Present queues are shared");
    } else {
        TE_LOG_INFO("  Graphics and Present queues are separate");
    }

    // 8. 创建命令池
    TE_LOG_INFO("[8] Creating command pool...");
    auto commandPool = device->CreateCommandPool(
        queueFamilies.graphics.value(),
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer
    );
    if (!commandPool) {
        TE_LOG_ERROR("Failed to create command pool");
        return -1;
    }
    TE_LOG_INFO(" Command pool created: QueueFamily={}",
                commandPool->GetQueueFamilyIndex());

    // 9. 分配命令缓冲
    TE_LOG_INFO("[9] Allocating command buffers...");
    constexpr uint32_t commandBufferCount = 3;
    auto commandBuffers = commandPool->AllocateBuffers(commandBufferCount);
    TE_LOG_INFO(" Allocated {} command buffer(s)", commandBuffers.size());

    // 10. 创建同步对象
    TE_LOG_INFO("[10] Creating synchronization objects...");
    auto imageAvailableSemaphore = device->CreateSemaphore();
    auto renderFinishedSemaphore = device->CreateSemaphore();
    auto inFlightFence = device->CreateFence(true);  // 初始为已信号状态
    TE_LOG_INFO(" Created semaphores and fence");

    // ============================================================
    // 测试队列操作
    // ============================================================

    TE_LOG_INFO("[11] Testing queue operations...");

    // 测试 WaitIdle
    TE_LOG_INFO("  Testing WaitIdle...");
    graphicsQueue->WaitIdle();
    TE_LOG_INFO("   Queue WaitIdle successful");

    // 测试设备 WaitIdle
    TE_LOG_INFO("  Testing device WaitIdle...");
    device->WaitIdle();
    TE_LOG_INFO("   Device WaitIdle successful");

    // 测试命令池 Reset
    TE_LOG_INFO("  Testing command pool reset...");
    commandPool->Reset();
    TE_LOG_INFO("   Command pool reset successful");

    // ============================================================
    // 层次3：资源对象 ✨ 新增测试
    // ============================================================

    // 12. 创建交换链
    TE_LOG_INFO("[12] Creating swap chain...");
    TE::SwapChainConfig swapChainConfig;
    swapChainConfig.preferredFormat = vk::Format::eB8G8R8A8Srgb;
    swapChainConfig.preferredPresentMode = vk::PresentModeKHR::eMailbox;
    swapChainConfig.imageCount = 3;

    auto swapChain = device->CreateSwapChain(
        *surface,
        swapChainConfig,
        window->GetWidth(),
        window->GetHeight()
    );
    if (!swapChain) {
        TE_LOG_ERROR("Failed to create swap chain");
        return -1;
    }
    TE_LOG_INFO("✓ Swap chain created:");
    TE_LOG_INFO("  Format: {}", vk::to_string(swapChain->GetFormat()));
    TE_LOG_INFO("  Extent: {}x{}", swapChain->GetExtent().width, swapChain->GetExtent().height);
    TE_LOG_INFO("  Image Count: {}", swapChain->GetImageCount());

    // 13. 创建 RenderPass
    TE_LOG_INFO("[13] Creating render pass...");
    std::vector<TE::AttachmentConfig> attachments;
    attachments.push_back({
        swapChain->GetFormat(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    });

    auto renderPass = device->CreateRenderPass(attachments);
    if (!renderPass) {
        TE_LOG_ERROR("Failed to create render pass");
        return -1;
    }
    TE_LOG_INFO("✓ Render pass created with {} attachment(s)", attachments.size());

    // 14. 为交换链图像创建 ImageView 和 Framebuffer
    TE_LOG_INFO("[14] Creating image views and framebuffers...");
    std::vector<std::unique_ptr<TE::VulkanImageView>> imageViews;
    std::vector<std::unique_ptr<TE::VulkanFramebuffer>> framebuffers;

    for (uint32_t i = 0; i < swapChain->GetImageCount(); ++i) {
        // 创建 ImageView
        auto imageView = swapChain->CreateImageView(i);
        if (!imageView) {
            TE_LOG_ERROR("Failed to create image view {}", i);
            return -1;
        }
        imageViews.push_back(std::move(imageView));

        // 创建 Framebuffer
        std::vector<vk::ImageView> fbAttachments = {*imageViews[i]->GetHandle()};
        auto framebuffer = device->CreateFramebuffer(
            *renderPass,
            fbAttachments,
            swapChain->GetExtent()
        );
        if (!framebuffer) {
            TE_LOG_ERROR("Failed to create framebuffer {}", i);
            return -1;
        }
        framebuffers.push_back(std::move(framebuffer));
    }
    TE_LOG_INFO("✓ Created {} image view(s) and framebuffer(s)", imageViews.size());

    // 15. 测试交换链操作
    TE_LOG_INFO("[15] Testing swap chain operations...");

    // 测试获取图像
    TE_LOG_INFO("  Testing AcquireNextImage...");
    auto [imageIndex, acquireResult] = swapChain->AcquireNextImage(*imageAvailableSemaphore);
    if (acquireResult == vk::Result::eSuccess || acquireResult == vk::Result::eSuboptimalKHR) {
        TE_LOG_INFO("  ✓ Acquired image: Index={}, Result={}", 
                   imageIndex, vk::to_string(acquireResult));
    } else {
        TE_LOG_WARN("  AcquireNextImage returned: {}", vk::to_string(acquireResult));
        return -1;
    }

    // 执行一个空的渲染通道来转换图像布局（从 UNDEFINED 到 PRESENT_SRC_KHR）
    // 这是必需的，因为呈现需要图像处于 PRESENT_SRC_KHR 布局
    TE_LOG_INFO("  Recording command buffer for layout transition...");
    auto& cmdBuffer = commandBuffers[0];
    
    // 开始记录命令
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdBuffer.begin(beginInfo);
    
    // 开始渲染通道（这会自动将图像从 UNDEFINED 转换为 COLOR_ATTACHMENT_OPTIMAL）
    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.setRenderPass(*renderPass->GetHandle());
    renderPassInfo.setFramebuffer(*framebuffers[imageIndex]->GetHandle());
    renderPassInfo.setRenderArea(vk::Rect2D({0, 0}, swapChain->GetExtent()));
    
    vk::ClearValue clearColor;
    clearColor.setColor(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));
    renderPassInfo.setClearValues(clearColor);
    
    cmdBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
    
    // 这里可以添加实际的绘制命令，但为了测试，我们只执行空的渲染通道
    
    // 结束渲染通道（这会自动将图像从 COLOR_ATTACHMENT_OPTIMAL 转换为 PRESENT_SRC_KHR）
    cmdBuffer.endRenderPass();
    
    // 结束记录
    cmdBuffer.end();
    
    // 提交命令缓冲区
    TE_LOG_INFO("  Submitting command buffer...");
    std::vector<vk::CommandBuffer> cmdBuffers = {*cmdBuffer};
    std::vector<vk::Semaphore> waitSemaphores = {*imageAvailableSemaphore};
    std::vector<vk::PipelineStageFlags> waitStages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    std::vector<vk::Semaphore> signalSemaphores = {*renderFinishedSemaphore};
    graphicsQueue->Submit(
        cmdBuffers,        // 命令缓冲区
        waitSemaphores,    // 等待信号量
        waitStages,        // 等待阶段
        signalSemaphores   // 信号量
    );
    
    // 测试呈现
    TE_LOG_INFO("  Testing Present...");
    vk::Result presentResult = swapChain->Present(
        imageIndex,
        *presentQueue,
        {*renderFinishedSemaphore}
    );
    if (presentResult == vk::Result::eSuccess || presentResult == vk::Result::eSuboptimalKHR) {
        TE_LOG_INFO("  ✓ Present successful: Result={}", vk::to_string(presentResult));
    } else {
        TE_LOG_WARN("  Present returned: {}", vk::to_string(presentResult));
    }

    // 等待队列完成
    presentQueue->WaitIdle();
    
    // 等待设备空闲，确保所有操作完成
    device->WaitIdle();

    // 16. 测试重建交换链
    TE_LOG_INFO("[16] Testing swap chain recreation...");
    
    // 先保存旧的交换链，然后显式销毁它
    // 这确保旧的交换链在创建新的之前完全销毁
    {
        auto oldSwapChain = std::move(swapChain);
        // oldSwapChain 在这里会被销毁，释放对表面的占用
    }
    
    // 现在可以安全地创建新的交换链
    swapChain = device->CreateSwapChain(
        *surface,
        swapChainConfig,
        window->GetWidth(),
        window->GetHeight()
    );
    
    if (swapChain) {
        TE_LOG_INFO("  ✓ Swap chain recreated successfully");
        TE_LOG_INFO("    New Format: {}", vk::to_string(swapChain->GetFormat()));
        TE_LOG_INFO("    New Extent: {}x{}", 
                   swapChain->GetExtent().width, 
                   swapChain->GetExtent().height);
    } else {
        TE_LOG_WARN("  Swap chain recreation failed");
    }

    // ============================================================
    // 主循环（简单测试）
    // ============================================================

    TE_LOG_INFO("[17] Entering main loop... (Press ESC or close window to exit)");
    TE_LOG_INFO("  Note: No actual rendering yet (Layer 4: CommandBuffer & Pipeline)");

    uint32_t frameCount = 0;
    while (!window->ShouldClose())
    {
        window->PollEvents();

        // 简单的帧计数测试
        frameCount++;
        if (frameCount % 60 == 0) {
            TE_LOG_DEBUG("Frame: {}", frameCount);
        }

        // 这里后续会添加实际的渲染代码（层次4：CommandBuffer 和 Pipeline）
    }

    TE_LOG_INFO("[18] Shutting down...");

    // 等待设备空闲（确保所有操作完成）
    device->WaitIdle();

    // 所有资源会自动销毁（RAII）
    // 销毁顺序：commandBuffers → commandPool → device → queues → ...

    TE_LOG_INFO(" Cleanup complete");

    TE_LOG_INFO("=== Test completed successfully ===");
    return 0;
}