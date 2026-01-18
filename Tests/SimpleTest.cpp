#include <fstream>
#include <iostream>
#include <vector>
#include "Log/Log.h"
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>
#include "glfw-3.4/include/GLFW/glfw3.h"

// 常量定义
constexpr auto WINDOW_WIDTH = 1280;
constexpr auto WINDOW_HEIGHT = 720;
constexpr auto swapChainImageFormat = vk::Format::eB8G8R8A8Srgb;
constexpr auto swapChainExtent = vk::Extent2D(WINDOW_WIDTH, WINDOW_HEIGHT);

// Vulkan 对象
GLFWwindow* window;
vk::raii::Context context;
vk::raii::Instance instance = VK_NULL_HANDLE;
vk::raii::SurfaceKHR surface = VK_NULL_HANDLE;
vk::raii::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
vk::raii::Device device = VK_NULL_HANDLE;
vk::raii::Queue graphicsQueue = VK_NULL_HANDLE;
vk::raii::Queue presentQueue = VK_NULL_HANDLE;
vk::raii::SwapchainKHR swapChain = VK_NULL_HANDLE;
std::vector<vk::raii::ImageView> swapChainImageViews;
vk::raii::RenderPass renderPass = VK_NULL_HANDLE;
std::vector<vk::raii::Framebuffer> framebuffers;
vk::raii::PipelineLayout pipelineLayout = VK_NULL_HANDLE;
vk::raii::Pipeline graphicsPipeline = VK_NULL_HANDLE;
vk::raii::CommandPool commandPool = VK_NULL_HANDLE;
std::vector<vk::raii::CommandBuffer> commandBuffers;
vk::raii::Semaphore imageAvailableSemaphore = VK_NULL_HANDLE;
vk::raii::Semaphore renderFinishedSemaphore = VK_NULL_HANDLE;
vk::raii::Fence inFlightFence = VK_NULL_HANDLE;

uint32_t graphicsQueueFamilyIndex = 0;
uint32_t presentQueueFamilyIndex = 0;

// 加载着色器
vk::raii::ShaderModule loadShader(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        TE_LOG_ERROR("Failed to open shader file: {}", filepath);
        throw std::runtime_error("Failed to open shader file");
    }

    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.setCode(buffer);
    return device.createShaderModule(createInfo);
}

// 初始化 GLFW 窗口
void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Triangle", nullptr, nullptr);
}

// 创建 Vulkan 实例
void createInstance() {
    vk::ApplicationInfo appInfo{};
    appInfo.setPApplicationName("Vulkan Triangle")
           .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
           .setPEngineName("No Engine")
           .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
           .setApiVersion(VK_API_VERSION_1_3);

    std::vector<const char*> extensions = {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

    vk::InstanceCreateInfo createInfo{};
    createInfo.setPApplicationInfo(&appInfo)
              .setPEnabledExtensionNames(extensions);

    instance = vk::raii::Instance(context, createInfo);
}

// 创建表面
void createSurface() {
    VkSurfaceKHR vkSurface;
    glfwCreateWindowSurface(*instance, window, nullptr, &vkSurface);
    surface = vk::raii::SurfaceKHR(instance, vkSurface);
}

// 选择物理设备
void pickPhysicalDevice() {
    vk::raii::PhysicalDevices physicalDevices(instance);
    physicalDevice = physicalDevices.front();
}

// 查找队列族
void findQueueFamilies() {
    auto queueFamilies = physicalDevice.getQueueFamilyProperties();

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            graphicsQueueFamilyIndex = i;
        }

        VkBool32 presentSupport = physicalDevice.getSurfaceSupportKHR(i, *surface);
        if (presentSupport) {
            presentQueueFamilyIndex = i;
        }
    }
}

// 创建逻辑设备
void createLogicalDevice() {
    float queuePriority = 1.0f;
    vk::DeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.setQueueFamilyIndex(graphicsQueueFamilyIndex)
                   .setQueueCount(1)
                   .setPQueuePriorities(&queuePriority);

    std::vector<const char*> deviceExtensions = { "VK_KHR_swapchain" };

    vk::DeviceCreateInfo createInfo{};
    createInfo.setQueueCreateInfos(queueCreateInfo)
              .setPEnabledExtensionNames(deviceExtensions);

    device = physicalDevice.createDevice(createInfo);
    graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);
    presentQueue = device.getQueue(presentQueueFamilyIndex, 0);
}

// 创建交换链
void createSwapChain() {
    auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(*surface);
    auto formats = physicalDevice.getSurfaceFormatsKHR(*surface);
    auto presentModes = physicalDevice.getSurfacePresentModesKHR(*surface);

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.setSurface(*surface)
              .setMinImageCount(2)
              .setImageFormat(swapChainImageFormat)
              .setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
              .setImageExtent(swapChainExtent)
              .setImageArrayLayers(1)
              .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
              .setImageSharingMode(vk::SharingMode::eExclusive)
              .setPreTransform(capabilities.currentTransform)
              .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
              .setPresentMode(vk::PresentModeKHR::eFifo)
              .setClipped(VK_TRUE);

    swapChain = device.createSwapchainKHR(createInfo);
}

// 创建图像视图
void createImageViews() {
    auto images = swapChain.getImages();

    for (auto& image : images) {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.setImage(image)
                  .setViewType(vk::ImageViewType::e2D)
                  .setFormat(swapChainImageFormat)
                  .setSubresourceRange({
                      vk::ImageAspectFlagBits::eColor,
                      0, 1, 0, 1
                  });

        swapChainImageViews.emplace_back(device.createImageView(createInfo));
    }
}

// 创建渲染通道
void createRenderPass() {
    vk::AttachmentDescription colorAttachment{};
    colorAttachment.setFormat(swapChainImageFormat)
                   .setSamples(vk::SampleCountFlagBits::e1)
                   .setLoadOp(vk::AttachmentLoadOp::eClear)
                   .setStoreOp(vk::AttachmentStoreOp::eStore)
                   .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                   .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                   .setInitialLayout(vk::ImageLayout::eUndefined)
                   .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    vk::AttachmentReference colorAttachmentRef{};
    colorAttachmentRef.setAttachment(0)
                      .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    vk::SubpassDescription subpass{};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
           .setColorAttachments(colorAttachmentRef);

    vk::SubpassDependency dependency{};
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
              .setDstSubpass(0)
              .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
              .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
              .setSrcAccessMask(vk::AccessFlagBits::eNone)
              .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

    vk::RenderPassCreateInfo renderPassInfo{};
    renderPassInfo.setAttachments(colorAttachment)
                  .setSubpasses(subpass)
                  .setDependencies(dependency);

    renderPass = device.createRenderPass(renderPassInfo);
}

// 创建图形管线
void createGraphicsPipeline() {
    // 加载着色器
    auto vertShaderModule = loadShader("");
    auto fragShaderModule = loadShader("");

    // 着色器阶段
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.setStage(vk::ShaderStageFlagBits::eVertex)
                       .setModule(*vertShaderModule)
                       .setPName("main");

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.setStage(vk::ShaderStageFlagBits::eFragment)
                       .setModule(*fragShaderModule)
                       .setPName("main");

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
        vertShaderStageInfo,
        fragShaderStageInfo
    };

    // 顶点输入（不需要顶点缓冲区，直接在着色器中生成）
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};

    // 输入装配
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList)
                 .setPrimitiveRestartEnable(VK_FALSE);

    // 视口和裁剪
    vk::Viewport viewport{};
    viewport.setX(0.0f)
            .setY(0.0f)
            .setWidth(static_cast<float>(WINDOW_WIDTH))
            .setHeight(static_cast<float>(WINDOW_HEIGHT))
            .setMinDepth(0.0f)
            .setMaxDepth(1.0f);

    vk::Rect2D scissor{};
    scissor.setOffset({0, 0})
           .setExtent(swapChainExtent);

    vk::PipelineViewportStateCreateInfo viewportState{};
    viewportState.setViewports(viewport)
                 .setScissors(scissor);

    // 光栅化
    vk::PipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.setDepthClampEnable(VK_FALSE)
              .setRasterizerDiscardEnable(VK_FALSE)
              .setPolygonMode(vk::PolygonMode::eFill)
              .setLineWidth(1.0f)
              .setCullMode(vk::CullModeFlagBits::eBack)
              .setFrontFace(vk::FrontFace::eClockwise)
              .setDepthBiasEnable(VK_FALSE);

    // 多重采样
    vk::PipelineMultisampleStateCreateInfo multisampling{};
    multisampling.setSampleShadingEnable(VK_FALSE)
                 .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    // 颜色混合
    vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA)
                        .setBlendEnable(VK_FALSE);

    vk::PipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.setLogicOpEnable(VK_FALSE)
                 .setAttachments(colorBlendAttachment);

    // 动态状态
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.setDynamicStates(dynamicStates);

    // 管线布局（空的，因为我们不需要 uniform 或 push constant）
    vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

    // 创建管线
    vk::GraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.setStages(shaderStages)
                .setPVertexInputState(&vertexInputInfo)
                .setPInputAssemblyState(&inputAssembly)
                .setPViewportState(&viewportState)
                .setPRasterizationState(&rasterizer)
                .setPMultisampleState(&multisampling)
                .setPColorBlendState(&colorBlending)
                .setPDynamicState(&dynamicState)
                .setLayout(*pipelineLayout)
                .setRenderPass(*renderPass)
                .setSubpass(0);

    graphicsPipeline = device.createGraphicsPipeline(nullptr, pipelineInfo);
}

// 创建帧缓冲区
void createFramebuffers() {
    for (auto& imageView : swapChainImageViews) {
        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.setRenderPass(*renderPass)
                       .setAttachments(*imageView)
                       .setWidth(WINDOW_WIDTH)
                       .setHeight(WINDOW_HEIGHT)
                       .setLayers(1);

        framebuffers.emplace_back(device.createFramebuffer(framebufferInfo));
    }
}

// 创建命令池和命令缓冲区
void createCommandBuffers() {
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
            .setQueueFamilyIndex(graphicsQueueFamilyIndex);

    commandPool = device.createCommandPool(poolInfo);

    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.setCommandPool(*commandPool)
             .setLevel(vk::CommandBufferLevel::ePrimary)
             .setCommandBufferCount(static_cast<uint32_t>(framebuffers.size()));

    commandBuffers = device.allocateCommandBuffers(allocInfo);

    // 记录命令
    for (size_t i = 0; i < commandBuffers.size(); ++i) {
        vk::CommandBufferBeginInfo beginInfo{};
        commandBuffers[i].begin(beginInfo);

        vk::ClearValue clearColor = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});

        vk::RenderPassBeginInfo renderPassInfo{};
        renderPassInfo.setRenderPass(*renderPass)
                      .setFramebuffer(*framebuffers[i])
                      .setRenderArea({{0, 0}, swapChainExtent})
                      .setClearValues(clearColor);

        commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
        commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

        // 设置动态状态
        vk::Viewport viewport{};
        viewport.setX(0.0f)
                .setY(0.0f)
                .setWidth(static_cast<float>(WINDOW_WIDTH))
                .setHeight(static_cast<float>(WINDOW_HEIGHT))
                .setMinDepth(0.0f)
                .setMaxDepth(1.0f);
        commandBuffers[i].setViewport(0, viewport);

        vk::Rect2D scissor{};
        scissor.setOffset({0, 0})
               .setExtent(swapChainExtent);
        commandBuffers[i].setScissor(0, scissor);

        // 绘制三角形（3个顶点）
        commandBuffers[i].draw(3, 1, 0, 0);

        commandBuffers[i].endRenderPass();
        commandBuffers[i].end();
    }
}

// 创建同步对象
void createSyncObjects() {
    vk::SemaphoreCreateInfo semaphoreInfo{};
    imageAvailableSemaphore = device.createSemaphore(semaphoreInfo);
    renderFinishedSemaphore = device.createSemaphore(semaphoreInfo);

    vk::FenceCreateInfo fenceInfo{};
    fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    inFlightFence = device.createFence(fenceInfo);
}

// 绘制帧
void drawFrame() {
    device.waitForFences(*inFlightFence, VK_TRUE, UINT64_MAX);
    device.resetFences(*inFlightFence);

    auto [result, imageIndex] = swapChain.acquireNextImage(UINT64_MAX, *imageAvailableSemaphore, nullptr);

    vk::SubmitInfo submitInfo{};
    vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.setWaitSemaphores(*imageAvailableSemaphore)
              .setWaitDstStageMask(waitStages)
              .setCommandBuffers(*commandBuffers[imageIndex])
              .setSignalSemaphores(*renderFinishedSemaphore);

    graphicsQueue.submit(submitInfo, *inFlightFence);

    vk::PresentInfoKHR presentInfo{};
    presentInfo.setWaitSemaphores(*renderFinishedSemaphore)
               .setSwapchains(*swapChain)
               .setImageIndices(imageIndex);

    presentQueue.presentKHR(presentInfo);
}

// 主函数
int main() {
    TE::Log::Init();

    try {
        initWindow();
        createInstance();
        createSurface();
        pickPhysicalDevice();
        findQueueFamilies();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandBuffers();
        createSyncObjects();

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            drawFrame();
        }

        device.waitIdle();
    } catch (const std::exception& e) {
        TE_LOG_ERROR("Error: {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}