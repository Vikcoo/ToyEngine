// ToyEngine Sandbox - 完整渲染流程测试
#include "Log/Log.h"
#include "Window.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandPool.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanImageView.h"
#include "VulkanPipeline.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanVertexInput.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstddef>
#include <vector>

#include "glfw-3.4/include/GLFW/glfw3.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb-master/stb_image.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

// 定义顶点结构体
struct Vertex {
    glm::vec3 position;  // 位置 (location = 0)
    glm::vec3 color;     // 颜色 (location = 1)
    glm::vec2 texCoord;

    static vk::VertexInputBindingDescription getBindingDescription() {
        vk::VertexInputBindingDescription bindingDescription;
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        return bindingDescription;
    }

    static std::vector<vk::VertexInputAttributeDescription>  getAttributeDescriptions() {
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions;
        attributeDescriptions.emplace_back(0 , 0 , vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position));
        attributeDescriptions.emplace_back(1 , 0 , vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color));
        attributeDescriptions.emplace_back(2 , 0 , vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord));
        return attributeDescriptions;
    }
};

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

int main()
{
    // 初始化日志系统
    TE::Log::Init();

    /* 1. 创建窗口 */
    const TE::WindowConfig config{"ToyEngine", 1280, 720, true};
    const auto window = TE::Window::Create(config);
    if (!window) {
        TE_LOG_ERROR("Failed to create window");
        return -1;
    }

    /* 2. 上下文与实例 */
    auto context = TE::VulkanContext::Create();
    if (!context) {
        TE_LOG_ERROR("Failed to create Vulkan Context");
        return -1;
    }

    /* 3. 表面 */
    auto surface = context->CreateSurface(*window);
    if (!surface) {
        TE_LOG_ERROR("Failed to create Surface");
        return -1;
    }

    /* 4. 物理设备 */
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

    /* 5. 队列族 找到一个支持图形绘制和呈现的队列族 */
    auto queueFamilies = bestDevice->FindQueueFamilies(surface.get());
    if (!queueFamilies.IsComplete()) {
        TE_LOG_ERROR("Required queue families not found");
        return -1;
    }
    TE_LOG_INFO("Graphics Queue Family: {}", queueFamilies.graphics.value());
    TE_LOG_INFO("Present Queue Family: {}", queueFamilies.present.value());
    if (queueFamilies.compute.has_value()) {
        TE_LOG_INFO("  Compute Queue Family: {}", queueFamilies.compute.value());
    }
    if (queueFamilies.transfer.has_value()) {
        TE_LOG_INFO("  Transfer Queue Family: {}", queueFamilies.transfer.value());
    }

    /* 6. 逻辑设备 */
    TE::DeviceConfig deviceConfig;
    deviceConfig.enabledFeatures.samplerAnisotropy = VK_TRUE;  // 启用各向异性过滤
    auto device = TE::VulkanDevice::Create(bestDevice, queueFamilies, deviceConfig);
    if (!device) {
        TE_LOG_ERROR("Failed to create logical device");
        return -1;
    }

    /* 7. 队列  取出逻辑设备创建时创建的队列并检查 */
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
    TE_LOG_INFO("Graphics Queue: Family={}, Index={}", graphicsQueue->GetFamilyIndex(), graphicsQueue->GetQueueIndex());
    TE_LOG_INFO("Present Queue: Family={}, Index={}", presentQueue->GetFamilyIndex(), presentQueue->GetQueueIndex());

    /* 8.交换链 */
    TE::SwapChainConfig swapChainConfig{
        vk::Format::eB8G8R8A8Srgb,
        vk::ColorSpaceKHR::eSrgbNonlinear,
        vk::PresentModeKHR::eMailbox,
        3,
        vk::ImageUsageFlagBits::eColorAttachment
    };
    auto swapChain = device->CreateSwapChain(surface, swapChainConfig, window->GetWidth(), window->GetHeight());
    if (!swapChain) {
        TE_LOG_ERROR("Failed to create swap chain");
        return -1;
    }

    /* 9.图像 */
    uint32_t actualImageCount = swapChain->GetImageCount();
    std::vector<std::unique_ptr<TE::VulkanImageView>> imageViews;
    imageViews.reserve(actualImageCount);
    for (uint32_t i = 0; i < actualImageCount; ++i) {
        auto imageView = swapChain->CreateImageView(i);
        if (!imageView) {
            TE_LOG_ERROR("Failed to create image view {}", i);
            return -1;
        }
        imageViews.push_back(std::move(imageView));
    }
    TE_LOG_INFO(" Created {} image view(s) (actual swap chain image count: {})",
                actualImageCount, actualImageCount);

    /* 10. RenderPass */
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
    TE_LOG_INFO("Render pass created with {} attachment(s)", attachments.size());

    /* 11. FrameBuffer */
    // 为每个实际创建的图像创建对应的 Framebuffer
    std::vector<std::unique_ptr<TE::VulkanFramebuffer>> framebuffers;
    framebuffers.reserve(actualImageCount);
    for (uint32_t i = 0; i < actualImageCount; ++i) {
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
    TE_LOG_INFO(" Created {} framebuffer(s)", actualImageCount);

    /* 图形管线 */
    TE::GraphicsPipelineConfig pipelineConfig;

    std::string vertexShaderPath = "Content/Shaders/Common/triangle.vert.spv";
    std::string fragmentShaderPath = "Content/Shaders/Common/triangle.frag.spv";
    std::string projectRoot = TE_PROJECT_ROOT_DIR;
    if (projectRoot.back() != '/' && projectRoot.back() != '\\') {
        projectRoot += "/";
    }
    std::string absVertexPath = projectRoot + vertexShaderPath;
    std::string absFragmentPath = projectRoot + fragmentShaderPath;

    // 检查文件是否存在
    std::ifstream testFile(absVertexPath);
    if (testFile.good()) {
        vertexShaderPath = absVertexPath;
        fragmentShaderPath = absFragmentPath;
        TE_LOG_INFO("Using absolute shader paths from project root");
    } else {
        TE_LOG_INFO("Using relative shader paths (current working directory)");
    }
    testFile.close();

    pipelineConfig.vertexShaderPath = vertexShaderPath;
    pipelineConfig.fragmentShaderPath = fragmentShaderPath;

    // 视口和剪裁
    pipelineConfig.viewport = vk::Viewport(
        0.0f, 0.0f,
        static_cast<float>(swapChain->GetExtent().width),
        static_cast<float>(swapChain->GetExtent().height),
        0.0f, 1.0f
    );
    pipelineConfig.scissor = vk::Rect2D({0, 0}, swapChain->GetExtent());

    // 配置顶点输入（使用辅助函数）
    pipelineConfig.vertexBindings = {
        Vertex::getBindingDescription()
    };
    pipelineConfig.vertexAttributes = 
        Vertex::getAttributeDescriptions();

    /* 描述符集布局（UBO） */
    // 创建描述符集布局，定义 UBO 绑定（binding = 0）
    std::vector<TE::DescriptorSetLayoutBinding> uboDescLayoutBindings;
    uboDescLayoutBindings.push_back({
        0,                                          // binding = 0（对应 shader 中的 layout(binding = 0)）
        vk::DescriptorType::eUniformBuffer,        // UBO 类型
        1,                                          // 数量
        vk::ShaderStageFlagBits::eVertex         // 在顶点着色器中使用
    });

    auto uboDescSetLayout = device->CreateDescriptorSetLayout(uboDescLayoutBindings);
    if (!uboDescSetLayout) {
        TE_LOG_ERROR("Failed to create descriptor set layout");
        return -1;
    }

    // 创建描述符集布局 纹理采样器（Combined Image Sampler）
    std::vector<TE::DescriptorSetLayoutBinding> samplerLayoutBindings;
    samplerLayoutBindings.push_back({
        0,                                          // binding = 0（对应 shader 中的 layout(binding = 0) set = 1
        vk::DescriptorType::eCombinedImageSampler,  // 组合图像采样器类型
        1,                                          // 数量
        vk::ShaderStageFlagBits::eFragment         // 在片段着色器中使用
    });

    auto samplerDescSetLayout = device->CreateDescriptorSetLayout(samplerLayoutBindings);
    if (!samplerDescSetLayout) {
        TE_LOG_ERROR("Failed to create sampler descriptor set layout");
        return -1;
    }

    pipelineConfig.descriptorSetLayouts = {
        *uboDescSetLayout->GetHandle(),      // set = 0
        *samplerDescSetLayout->GetHandle()    // set = 1
    };
    auto pipeline = device->CreateGraphicsPipeline(*renderPass, pipelineConfig);
    if (!pipeline) {
        TE_LOG_ERROR("Failed to create graphics pipeline");
        return -1;
    }
    TE_LOG_INFO(" Graphics pipeline created");
    TE_LOG_INFO("  Viewport: {}x{}", swapChain->GetExtent().width, swapChain->GetExtent().height);
    TE_LOG_INFO("  Shaders: {} + {}", pipelineConfig.vertexShaderPath, pipelineConfig.fragmentShaderPath);

    /* 顶点缓冲区 */
    // 定义顶点数据
    const std::vector<Vertex> vertices = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };
    const std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

    // 创建顶点缓冲区配置（使用设备本地内存 - 显存）
    // 这是最佳实践：静态顶点数据应该存储在 GPU 显存中，以获得最快访问速度
    TE::BufferConfig vertexBufferConfig;
    vertexBufferConfig.size = sizeof(Vertex) * vertices.size();
    vertexBufferConfig.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    // 创建顶点缓冲区（在显存中）
    auto vertexBuffer = device->CreateBuffer(vertexBufferConfig);
    if (!vertexBuffer) {
        TE_LOG_ERROR("Failed to create vertex buffer");
        return -1;
    }

    TE::BufferConfig indexBufferConfig;
    indexBufferConfig.size = sizeof(uint32_t) * indices.size();
    indexBufferConfig.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst;
    auto indexBuffer = device->CreateBuffer(indexBufferConfig);
    if (!indexBuffer){
        TE_LOG_ERROR("Failed to create index buffer");
        return -1;
    }

    // 使用 Staging Buffer 上传数据到显存
    // 这是 Vulkan 的最佳实践：CPU 数据 → Staging Buffer（主机可见）→ GPU 复制 → Device Buffer（显存）
    device->UploadToDeviceLocalBuffer(*vertexBuffer, vertices.data(), vertexBufferConfig.size);
    TE_LOG_INFO(" Vertex buffer created in device local memory: {} vertices ({} bytes)", 
                vertices.size(), vertexBufferConfig.size);

    device->UploadToDeviceLocalBuffer(*indexBuffer, indices.data(), indexBufferConfig.size);
    TE_LOG_INFO(" index buffer created in device local memory: {} vertices ({} bytes)",
                indices.size(), indexBufferConfig.size);


    constexpr int MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t currentFrame = 0;


    /* UBO（Uniform Buffer Object） */
    // 为每个交换链图像创建一个 UBO 缓冲区（主机可见内存，每帧更新）
    std::vector<std::unique_ptr<TE::VulkanBuffer>> uniformBuffers;
    uniformBuffers.reserve(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        TE::BufferConfig uboConfig;
        uboConfig.size = sizeof(UniformBufferObject);
        uboConfig.usage = vk::BufferUsageFlagBits::eUniformBuffer;
        uboConfig.memoryProperties = 
            vk::MemoryPropertyFlagBits::eHostVisible |      // CPU 可访问（每帧更新）
            vk::MemoryPropertyFlagBits::eHostCoherent;      // 主机一致（自动同步到 GPU）

        uniformBuffers.push_back(device->CreateBuffer(uboConfig));
        if (!uniformBuffers.back()) {
            TE_LOG_ERROR("Failed to create uniform buffer {}", i);
            return -1;
        }
    }
    TE_LOG_INFO(" Created {} uniform buffer(s)", MAX_FRAMES_IN_FLIGHT);

    /* 描述符池 */
    // 创建描述符池，用于分配描述符集
    std::vector<TE::DescriptorPoolSize> poolSizes;
    poolSizes.push_back({vk::DescriptorType::eUniformBuffer, static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)});
    poolSizes.push_back({vk::DescriptorType::eCombinedImageSampler,static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT)});

    auto descriptorPool = device->CreateDescriptorPool(MAX_FRAMES_IN_FLIGHT * 2, poolSizes);
    if (!descriptorPool) {
        TE_LOG_ERROR("Failed to create descriptor pool");
        return -1;
    }
    TE_LOG_INFO(" Descriptor pool created");

    /* 描述符集 */
    // 为每个交换链图像分配一个描述符集
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, *uboDescSetLayout->GetHandle());
    std::vector<vk::raii::DescriptorSet> uboDescriptorSets = descriptorPool->AllocateDescriptorSets(layouts);
    
    if (uboDescriptorSets.size() != MAX_FRAMES_IN_FLIGHT) {
        TE_LOG_ERROR("Failed to allocate descriptor sets");
        return -1;
    }
    TE_LOG_INFO(" Allocated {} descriptor set(s)", uboDescriptorSets.size());

    // 分配纹理采样器描述符集（每个帧一个，但纹理是共享的）
    std::vector<vk::DescriptorSetLayout> samplerLayouts(MAX_FRAMES_IN_FLIGHT, *samplerDescSetLayout->GetHandle());
    std::vector<vk::raii::DescriptorSet> samplerDescriptorSets = descriptorPool->AllocateDescriptorSets(samplerLayouts);
    if (samplerDescriptorSets.size() != MAX_FRAMES_IN_FLIGHT) {
        TE_LOG_ERROR("Failed to allocate sampler descriptor sets");
        return -1;
    }

    auto texture = device->CreateTexture2DFromfile("C:/Project Files/ToyEngine/Content/Textures/rust_cpp.png");
    if (!texture) {
        TE_LOG_ERROR("Failed to load texture");
        return -1;
    }

    // 更新描述符集，将 UBO 绑定到描述符集
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.setBuffer(uniformBuffers[i]->GetHandle())
                  .setOffset(0)
                  .setRange(sizeof(UniformBufferObject));

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.setDstSet(uboDescriptorSets[i])
                       .setDstBinding(0)                    // binding = 0
                       .setDstArrayElement(0)
                       .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                       .setDescriptorCount(1)
                       .setBufferInfo({bufferInfo});

        // 更新纹理采样器描述符集（set 1）
        vk::DescriptorImageInfo imageInfo;
        imageInfo.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                 .setImageView(*texture->GetImageView().GetHandle())  // 需要添加 GetImageView 方法
                 .setSampler(*texture->GetSampler());                  // 需要添加 GetSampler 方法

        vk::WriteDescriptorSet samplerWrite;
        samplerWrite.setDstSet(*samplerDescriptorSets[i])  // ✅ 修复：使用解引用
                    .setDstBinding(0)
                    .setDstArrayElement(0)
                    .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                    .setDescriptorCount(1)
                    .setImageInfo({imageInfo});

        device->GetHandle().updateDescriptorSets({descriptorWrite, samplerWrite}, {});
    }
    TE_LOG_INFO(" Descriptor sets updated");

    /* 命令池 */
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



    /* 命令缓冲 */
    // 为每个实际创建的图像分配命令缓冲区
    auto commandBuffers = commandPool->AllocateCommandBuffers(MAX_FRAMES_IN_FLIGHT);
    if (commandBuffers.size() < MAX_FRAMES_IN_FLIGHT) {
        TE_LOG_ERROR("Failed to allocate enough command buffers");
        return -1;
    }
    TE_LOG_INFO(" Allocated {} command buffer(s)", commandBuffers.size());



    /* 同步对象 */
    // 注意：即使设置了 imageCount=1，驱动可能强制最小图像数量（通常是2）
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
    std::vector<vk::raii::Fence> inFlightFences;
    imageAvailableSemaphores.reserve(MAX_FRAMES_IN_FLIGHT);

    inFlightFences.reserve(MAX_FRAMES_IN_FLIGHT);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        imageAvailableSemaphores.push_back(device->CreateVulkanSemaphore());
        inFlightFences.push_back(device->CreateFence());
    }

    renderFinishedSemaphores.reserve(actualImageCount);
    for (uint32_t i = 0; i < actualImageCount; ++i) {
        renderFinishedSemaphores.push_back(device->CreateVulkanSemaphore());
    }

    /* 清除颜色值 */
    vk::ClearValue clearColor;
    clearColor.setColor(vk::ClearColorValue(std::array<float, 4>{0.1f, 0.1f, 0.1f, 1.0f}));

    /* 窗口状态管理 */
    // 用于标记窗口大小改变和窗口最小化状态（事件驱动，最优雅的方式）
    std::atomic<bool> framebufferResized{false};
    std::atomic<bool> windowMinimized{false};

    // 设置窗口大小改变回调（事件驱动，最优雅的方式）
    window->SetResizeCallback([&framebufferResized, &windowMinimized](uint32_t width, uint32_t height) {
        // 窗口大小改变时，标记需要重建交换链
        framebufferResized = true;
        // 更新窗口最小化状态（当窗口尺寸为0时，通常表示最小化）
        windowMinimized = (width == 0 || height == 0);
        TE_LOG_DEBUG("Framebuffer resized: {}x{} (minimized: {})", width, height, windowMinimized.load());
    });

    // 设置窗口图标化回调（正确处理最小化/恢复，这是最准确的方式）
    window->SetIconifyCallback([&windowMinimized](bool iconified) {
        windowMinimized = iconified;
        if (iconified) {
            TE_LOG_DEBUG("Window minimized (iconified)");
        } else {
            TE_LOG_DEBUG("Window restored from minimized (uniconified)");
        }
    });

    /* 封装交换链重建逻辑（优雅的函数封装，避免代码重复） */
    // 注意：使用 [&] 捕获所有引用，包括上面声明的变量
    auto RecreateSwapChain = [&]() -> bool {
        // 检查窗口是否最小化
        if (window->GetWidth() == 0 || window->GetHeight() == 0) {
            windowMinimized = true;  // 更新状态
            TE_LOG_DEBUG("Window minimized, skipping swap chain recreation");
            return false;  // 窗口最小化，不重建
        }

        TE_LOG_WARN("Recreating swap chain...");

        // 1. 等待设备空闲（确保所有命令执行完毕）
        device->WaitIdle();

        // 2. 清理旧资源（自动析构）
        imageViews.clear();
        framebuffers.clear();
        commandBuffers.clear();
        renderFinishedSemaphores.clear();

        // 3. 重建交换链（使用新的窗口尺寸）
        auto newSwapChain = swapChain->Recreate(
            swapChainConfig,
            window->GetWidth(),   // 使用新的窗口宽度
            window->GetHeight()  // 使用新的窗口高度
        );
        if (!newSwapChain) {
            TE_LOG_ERROR("Failed to recreate swap chain");
            return false;
        }
        swapChain = std::move(newSwapChain);

        // 4. 更新实际图像数量
        actualImageCount = swapChain->GetImageCount();

        // 5. 重新创建 ImageView
        imageViews.clear();
        imageViews.reserve(actualImageCount);
        for (uint32_t i = 0; i < actualImageCount; ++i) {
            auto imageView = swapChain->CreateImageView(i);
            if (!imageView) {
                TE_LOG_ERROR("Failed to create image view {}", i);
                return false;
            }
            imageViews.push_back(std::move(imageView));
        }

        // 6. 重新创建 Framebuffer
        framebuffers.clear();
        framebuffers.reserve(actualImageCount);
        for (uint32_t i = 0; i < actualImageCount; ++i) {
            std::vector<vk::ImageView> fbAttachments = {*imageViews[i]->GetHandle()};
            auto framebuffer = device->CreateFramebuffer(
                *renderPass,
                fbAttachments,
                swapChain->GetExtent()
            );
            if (!framebuffer) {
                TE_LOG_ERROR("Failed to create framebuffer {}", i);
                return false;
            }
            framebuffers.push_back(std::move(framebuffer));
        }

        // 7. 重新分配命令缓冲区（数量不变，因为按 MAX_FRAMES_IN_FLIGHT）
        commandBuffers = commandPool->AllocateCommandBuffers(MAX_FRAMES_IN_FLIGHT);
        if (commandBuffers.size() < MAX_FRAMES_IN_FLIGHT) {
            TE_LOG_ERROR("Failed to allocate enough command buffers after swap chain recreation");
            return false;
        }

        // 8. 重新创建 renderFinishedSemaphores（按新的图像数量）
        renderFinishedSemaphores.clear();
        renderFinishedSemaphores.reserve(actualImageCount);
        for (uint32_t i = 0; i < actualImageCount; ++i) {
            renderFinishedSemaphores.push_back(device->CreateVulkanSemaphore());
        }

        // 9. 更新视口和剪裁配置（用于管线，虽然使用动态视口，但保持配置同步）
        pipelineConfig.viewport = vk::Viewport(
            0.0f,
            0.0f,
            static_cast<float>(swapChain->GetExtent().width),
            static_cast<float>(swapChain->GetExtent().height),
            0.0f, 1.0f
        );
        pipelineConfig.scissor = vk::Rect2D({0, 0}, swapChain->GetExtent());

        // 10. 清除标志
        framebufferResized = false;
        windowMinimized = false;  // 窗口已恢复，更新状态

        TE_LOG_INFO("Swap chain recreated successfully: {}x{}, {} images",
                    swapChain->GetExtent().width,
                    swapChain->GetExtent().height,
                    actualImageCount);
        return true;
    };

    while (!window->ShouldClose())
    {
        // 1. 处理窗口事件（必须每帧调用，否则窗口会无响应）
        window->PollEvents();

        // 2. 等待上一帧完成（CPU 等待 GPU）
        //    这确保我们不会提交超过 GPU 能处理的帧数，避免内存占用过高
        auto waitResult = device->GetHandle().waitForFences(*inFlightFences[currentFrame], true, UINT64_MAX);
        if (waitResult != vk::Result::eSuccess) {
            TE_LOG_ERROR("Failed to wait for fence: {}", vk::to_string(waitResult));
            break;
        }

        // 检查是否需要重建交换链
        if (framebufferResized) {
            if (!RecreateSwapChain()) {
                continue;
            }
        }

        // 如果窗口最小化，跳过渲染
        if (windowMinimized || window->GetWidth() == 0 || window->GetHeight() == 0) {
            windowMinimized = true;
            continue;
        }



        // 3. 从交换链获取下一个可用的图像索引
        //    imageAvailableSemaphore 会在图像准备好时被 GPU 发出信号
        auto acquireResult = swapChain->AcquireNextImage(
            *imageAvailableSemaphores[currentFrame],  // 信号量：图像可用时发出信号
            nullptr                     // 围栏：这里不需要，使用信号量即可
        );

        // 处理 AcquireNextImage 结果
        if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
            // 交换链过时，重建（优雅的统一处理）
            if (!RecreateSwapChain()) {
                // 窗口最小化，跳过这一帧
                continue;
            }
            // 重建成功，重新获取图像（下一帧）
            continue;
        } else if (acquireResult.result == vk::Result::eSuboptimalKHR) {
            // 交换链不是最优的，但可以继续使用
            TE_LOG_DEBUG("Swap chain is suboptimal");
        } else if (acquireResult.result != vk::Result::eSuccess) {
            // 其他错误
            TE_LOG_ERROR("Failed to acquire swap chain image: {}", vk::to_string(acquireResult.result));
            break;
        }

        // 重置围栏为未信号状态（准备下一帧）
        device->GetHandle().resetFences(*inFlightFences[currentFrame]);

        // 更新UBO（每帧更新）
        UniformBufferObject ubo{};
        // 模型矩阵：绕 Z 轴旋转（使用时间作为旋转角度）
        float time = static_cast<float>(glfwGetTime());
        ubo.model = glm::rotate(glm::mat4(1.0f), 
                                time * glm::radians(90.0f),
                                glm::vec3(0.0f, 0.0f, 1.0f));
        // 视图矩阵：从 (2, 2, 2) 看向原点
        ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                               glm::vec3(0.0f, 0.0f, 0.0f),
                               glm::vec3(0.0f, 0.0f, 1.0f));
        // 投影矩阵：透视投影
        ubo.proj = glm::perspective(glm::radians(45.0f),
                                    static_cast<float>(swapChain->GetExtent().width) / 
                                    static_cast<float>(swapChain->GetExtent().height),
                                    0.1f, 10.0f);
        // Vulkan 使用 Y 轴向下，需要翻转投影矩阵的 Y 轴
        ubo.proj[1][1] *= -1;

        // 上传 UBO 数据到当前帧的缓冲区（主机可见内存，直接写入）
        uniformBuffers[currentFrame]->UploadData(&ubo, sizeof(ubo));

        // 4. 录制命令缓冲区（使用 RAII Scope 自动管理 begin/end）
        {
            // 开始录制命令缓冲区
            auto recording = commandBuffers[currentFrame]->BeginRecording(
                vk::CommandBufferUsageFlagBits::eOneTimeSubmit  // eOneTimeSubmit 表示这个命令缓冲区只提交一次
            );

            // 开始渲染通道（使用 RAII Scope 自动管理 beginRenderPass/endRenderPass）
            {
                auto renderPassScope = commandBuffers[currentFrame]->BeginRenderPass(
                    *renderPass,                                    // 渲染通道
                    *framebuffers[acquireResult.imageIndex],        // 帧缓冲区（使用对应图像索引）
                    {clearColor}                                 // 清除颜色值
                );

                // 绑定图形管线 这告诉 GPU 使用哪个着色器程序和渲染状态
                commandBuffers[currentFrame]->BindPipeline(*pipeline);

                // 绑定描述符集（在绑定管线之后） 将 UBO 绑定到着色器，使着色器可以访问 uniform 数据
                commandBuffers[currentFrame]->BindDescriptorSets(
                    vk::PipelineBindPoint::eGraphics,
                    pipeline->GetLayout().operator*(),  // 获取 PipelineLayout
                    0,                                   // 第一个描述符集
                    {uboDescriptorSets[currentFrame], samplerDescriptorSets[currentFrame]}      // 当前帧的描述符集
                );

                // 绑定顶点缓冲区和索引缓冲区 ，供 GPU 读取
                commandBuffers[currentFrame]->BindVertexBuffer(0, *vertexBuffer, 0);
                commandBuffers[currentFrame]->BindIndexBuffer(*indexBuffer);

                // 设置视口（动态视口，会覆盖管线创建时的设置）视口定义了渲染区域在窗口中的位置和大小
                //    注意：Vulkan的Y轴默认从底部开始，需要翻转Y轴以匹配OpenGL风格
                vk::Viewport viewport;
                viewport.x = 0.0f;
                //viewport.y = static_cast<float>(swapChain->GetExtent().height);  // Y坐标从底部开始
                viewport.y = 0.0f,
                viewport.width = static_cast<float>(swapChain->GetExtent().width);
                //viewport.height = -static_cast<float>(swapChain->GetExtent().height);  // 负高度翻转Y轴
                viewport.height = static_cast<float>(swapChain->GetExtent().height);  // 负高度翻转Y轴
                viewport.minDepth = 0.0f;
                viewport.maxDepth = 1.0f;
                commandBuffers[currentFrame]->SetViewport(viewport);

                // 8. 设置剪裁矩形（可选，如果管线支持动态剪裁）
                //    剪裁矩形定义了哪些像素会被渲染（通常与视口相同）
                vk::Rect2D scissor;
                scissor.offset = vk::Offset2D{0, 0};
                scissor.extent = swapChain->GetExtent();
                commandBuffers[currentFrame]->SetScissor(scissor);

                // 9. 绘制三角形
                //commandBuffers[currentFrame]->Draw(3, 1, 0, 0);
                commandBuffers[currentFrame]->DrawIndexed(indices.size());

            }  // 自动调用 endRenderPass（RenderPassScope 析构）
        }  // 自动调用 end（CommandBufferRecordingScope 析构）

        // 10. 提交命令缓冲区到图形队列
        //     提交时需要指定：
        //     - 等待哪些信号量（等待图像可用）
        //     - 在哪个管线阶段等待（颜色附件输出阶段）
        //     - 发出哪些信号量（渲染完成时发出信号）
        //     - 围栏（用于 CPU 等待 GPU 完成）
        //  使用对应图像索引的信号量，避免信号量重用冲突
        graphicsQueue->Submit(
            {commandBuffers[currentFrame]->GetRawHandle()},      // 命令缓冲区（使用对应图像索引）
            {*imageAvailableSemaphores[currentFrame]},                          // 等待信号量：等待图像可用
            {vk::PipelineStageFlagBits::eColorAttachmentOutput}, // 等待阶段：在颜色附件输出阶段等待
            {*renderFinishedSemaphores[acquireResult.imageIndex]},            //  使用对应图像索引的信号量
            *inFlightFences[currentFrame]                                     // 围栏：用于 CPU 等待
        );

        // 11. 呈现图像到屏幕
        //     Present 会等待 renderFinishedSemaphore 信号，确保渲染完成后再呈现
        //     使用对应图像索引的信号量，避免信号量重用冲突
        vk::Result presentResult = swapChain->Present(
            acquireResult.imageIndex,                              // 要呈现的图像索引
            *presentQueue,                           // 呈现队列
            {*renderFinishedSemaphores[acquireResult.imageIndex]} //  使用对应图像索引的信号量
        );

        // 检查呈现结果
        if (presentResult == vk::Result::eErrorOutOfDateKHR ||
            presentResult == vk::Result::eSuboptimalKHR) {
            // Present 返回 ErrorOutOfDateKHR，标记需要重建（在下一帧处理，事件驱动）
            framebufferResized = true;
            TE_LOG_DEBUG("Swap chain out of date during present (will recreate next frame)");
        } else if (presentResult != vk::Result::eSuccess) {
            TE_LOG_ERROR("Failed to present swap chain image: {}", vk::to_string(presentResult));
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    // 12. 等待设备完成所有操作（清理前确保所有命令执行完毕）
    device->WaitIdle();
    return 0;
}