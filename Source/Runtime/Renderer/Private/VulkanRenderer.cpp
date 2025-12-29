// Source/Runtime/Renderer/Private/VulkanRenderer.cpp
#define NOMINMAX
#include "VulkanRenderer.h"
#include "VulkanContext.h"
#include "VulkanDevice.h"
#include "VulkanPhysicalDevice.h"
#include "VulkanSurface.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanPipeline.h"
#include "VulkanFramebuffer.h"
#include "VulkanImageView.h"
#include "VulkanCommandPool.h"
#include "VulkanCommandBuffer.h"
#include "VulkanBuffer.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"
#include "VulkanTexture2D.h"
#include "VulkanVertexInput.h"
#include "Log/Log.h"
#include <algorithm>
#include <atomic>
#include <fstream>

#include "VulkanQueue.h"

namespace TE {

VulkanRenderer::VulkanRenderer() = default;

VulkanRenderer::~VulkanRenderer() {
    Shutdown();
}

bool VulkanRenderer::Initialize(std::shared_ptr<Window> window, const RendererConfig& config) {
    if (m_initialized) {
        TE_LOG_WARN("Renderer already initialized");
        return false;
    }

    m_window = window;
    m_config = config;

    // 设置窗口回调
    m_window->SetResizeCallback([this](uint32_t width, uint32_t height) {
        m_framebufferResized = true;
        OnWindowResize(width, height);
    });

    if (!InitializeVulkan()) {
        TE_LOG_ERROR("Failed to initialize Vulkan");
        return false;
    }

    m_initialized = true;
    TE_LOG_INFO("VulkanRenderer initialized successfully");
    return true;
}

void VulkanRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }

    if (m_device) {
        m_device->WaitIdle();
    }

    CleanupSwapChain();

    // 清理同步对象
    m_inFlightFences.clear();
    m_renderFinishedSemaphores.clear();
    m_imageAvailableSemaphores.clear();
    m_imagesInFlight.clear();

    // 清理描述符
    m_descriptorPool.reset();
    m_samplerDescriptorSetLayout.reset();
    m_uboDescriptorSetLayout.reset();

    // 清理 UBO
    m_uniformBuffers.clear();

    // 清理命令缓冲区
    m_commandBuffers.clear();

    // 清理管线
    m_pipeline.reset();
    m_renderPass.reset();

    // 清理设备
    m_device.reset();
    m_context.reset();

    m_initialized = false;
    TE_LOG_INFO("VulkanRenderer shutdown");
}

bool VulkanRenderer::InitializeVulkan() {
    // 1. 创建 Context
    m_context = VulkanContext::Create();
    if (!m_context) {
        TE_LOG_ERROR("Failed to create Vulkan Context");
        return false;
    }

    // 2. 创建 Surface
    auto surface = m_context->CreateSurface(*m_window);
    if (!surface) {
        TE_LOG_ERROR("Failed to create Surface");
        return false;
    }

    // 3. 选择物理设备
    auto devices = m_context->EnumeratePhysicalDevices();
    if (devices.empty()) {
        TE_LOG_ERROR("No physical devices found");
        return false;
    }
    auto bestDevice = *std::max_element(devices.begin(), devices.end(),
        [](const auto& a, const auto& b) {
            return a->CalculateScore() < b->CalculateScore();
        });
    bestDevice->PrintInfo();

    // 4. 查找队列族
    auto queueFamilies = bestDevice->FindQueueFamilies(surface.get());
    if (!queueFamilies.IsComplete()) {
        TE_LOG_ERROR("Required queue families not found");
        return false;
    }

    // 5. 创建逻辑设备
    DeviceConfig deviceConfig;
    deviceConfig.enabledFeatures.samplerAnisotropy = VK_TRUE;
    m_device = VulkanDevice::Create(bestDevice, queueFamilies, deviceConfig);
    if (!m_device) {
        TE_LOG_ERROR("Failed to create logical device");
        return false;
    }

    // 6. 获取队列
    m_graphicsQueue = m_device->GetGraphicsQueue();
    m_presentQueue = m_device->GetPresentQueue();
    if (!m_graphicsQueue || !m_presentQueue) {
        TE_LOG_ERROR("Failed to get queues");
        return false;
    }

    // 7. 创建交换链
    if (!CreateSwapChain()) {
        return false;
    }

    // 8. 创建 RenderPass
    if (!CreateRenderPass()) {
        return false;
    }

    // 9. 创建 Pipeline
    if (!CreatePipeline()) {
        return false;
    }

    // 10. 创建 Framebuffers
    if (!CreateFramebuffers()) {
        return false;
    }

    // 11. 创建命令缓冲区
    if (!CreateCommandBuffers()) {
        return false;
    }

    // 12. 创建同步对象
    if (!CreateSyncObjects()) {
        return false;
    }

    // 13. 创建 UBO
    if (!CreateUniformBuffers()) {
        return false;
    }

    // 14. 创建描述符集
    if (!CreateDescriptorSets()) {
        return false;
    }

    return true;
}

bool VulkanRenderer::CreateSwapChain() {
    SwapChainConfig swapChainConfig{
        vk::Format::eB8G8R8A8Srgb,
        vk::ColorSpaceKHR::eSrgbNonlinear,
        m_config.enableVSync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eMailbox,
        m_config.swapChainImageCount,
        vk::ImageUsageFlagBits::eColorAttachment
    };

    auto surface = m_context->CreateSurface(*m_window);
    m_swapChain = m_device->CreateSwapChain(
        surface,
        swapChainConfig,
        m_window->GetWidth(),
        m_window->GetHeight()
    );

    return m_swapChain != nullptr;
}

bool VulkanRenderer::CreateRenderPass() {
    std::vector<AttachmentConfig> attachments;
    attachments.push_back({
        m_swapChain->GetFormat(),
        vk::SampleCountFlagBits::e1,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::ePresentSrcKHR
    });

    m_renderPass = m_device->CreateRenderPass(attachments);
    return m_renderPass != nullptr;
}

bool VulkanRenderer::CreatePipeline() {
    GraphicsPipelineConfig pipelineConfig;

    // Shader 路径处理
    std::string vertexShaderPath = m_config.vertexShaderPath;
    std::string fragmentShaderPath = m_config.fragmentShaderPath;
    std::string projectRoot = TE_PROJECT_ROOT_DIR;
    if (projectRoot.back() != '/' && projectRoot.back() != '\\') {
        projectRoot += "/";
    }
    std::string absVertexPath = projectRoot + vertexShaderPath;
    std::string absFragmentPath = projectRoot + fragmentShaderPath;

    std::ifstream testFile(absVertexPath);
    if (testFile.good()) {
        vertexShaderPath = absVertexPath;
        fragmentShaderPath = absFragmentPath;
    }
    testFile.close();

    pipelineConfig.vertexShaderPath = vertexShaderPath;
    pipelineConfig.fragmentShaderPath = fragmentShaderPath;

    // 视口和剪裁
    pipelineConfig.viewport = vk::Viewport(
        0.0f, 0.0f,
        static_cast<float>(m_swapChain->GetExtent().width),
        static_cast<float>(m_swapChain->GetExtent().height),
        0.0f, 1.0f
    );
    pipelineConfig.scissor = vk::Rect2D({0, 0}, m_swapChain->GetExtent());

    // 顶点输入（需要根据实际顶点类型配置）
    // 这里需要用户提供顶点类型信息，暂时留空
    // pipelineConfig.vertexBindings = ...
    // pipelineConfig.vertexAttributes = ...

    // 描述符集布局
    std::vector<DescriptorSetLayoutBinding> uboBindings;
    uboBindings.push_back({
        0,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eVertex
    });

    m_uboDescriptorSetLayout = m_device->CreateDescriptorSetLayout(uboBindings);
    if (!m_uboDescriptorSetLayout) {
        return false;
    }

    std::vector<DescriptorSetLayoutBinding> samplerBindings;
    samplerBindings.push_back({
        0,
        vk::DescriptorType::eCombinedImageSampler,
        1,
        vk::ShaderStageFlagBits::eFragment
    });

    m_samplerDescriptorSetLayout = m_device->CreateDescriptorSetLayout(samplerBindings);
    if (!m_samplerDescriptorSetLayout) {
        return false;
    }

    pipelineConfig.descriptorSetLayouts = {
        *m_uboDescriptorSetLayout->GetHandle(),
        *m_samplerDescriptorSetLayout->GetHandle()
    };

    m_pipeline = m_device->CreateGraphicsPipeline(*m_renderPass, pipelineConfig);
    return m_pipeline != nullptr;
}

bool VulkanRenderer::CreateFramebuffers() {
    uint32_t imageCount = m_swapChain->GetImageCount();
    m_framebuffers.reserve(imageCount);

    for (uint32_t i = 0; i < imageCount; ++i) {
        auto imageView = m_swapChain->CreateImageView(i);
        if (!imageView) {
            TE_LOG_ERROR("Failed to create image view {}", i);
            return false;
        }

        std::vector<vk::ImageView> attachments = {*imageView->GetHandle()};
        auto framebuffer = m_device->CreateFramebuffer(
            *m_renderPass,
            attachments,
            m_swapChain->GetExtent()
        );

        if (!framebuffer) {
            TE_LOG_ERROR("Failed to create framebuffer {}", i);
            return false;
        }

        m_framebuffers.push_back(std::move(framebuffer));
    }

    return true;
}

bool VulkanRenderer::CreateCommandBuffers() {
    auto queueFamilies = m_device->GetPhysicalDevice().FindQueueFamilies(
        m_context->CreateSurface(*m_window).get()
    );

    auto commandPool = m_device->CreateCommandPool(
        queueFamilies.graphics.value(),
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer
    );

    if (!commandPool) {
        return false;
    }

    m_commandBuffers = commandPool->AllocateCommandBuffers(m_config.maxFramesInFlight);
    return m_commandBuffers.size() >= m_config.maxFramesInFlight;
}

bool VulkanRenderer::CreateSyncObjects() {
    m_imageAvailableSemaphores.reserve(m_config.maxFramesInFlight);
    m_inFlightFences.reserve(m_config.maxFramesInFlight);

    for (uint32_t i = 0; i < m_config.maxFramesInFlight; ++i) {
        m_imageAvailableSemaphores.push_back(m_device->CreateVulkanSemaphore());
        m_inFlightFences.push_back(m_device->CreateFence());
    }

    uint32_t imageCount = m_swapChain->GetImageCount();
    m_renderFinishedSemaphores.reserve(imageCount);
    m_imagesInFlight.resize(imageCount, nullptr);

    for (uint32_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.push_back(m_device->CreateVulkanSemaphore());
    }

    return true;
}

bool VulkanRenderer::CreateUniformBuffers() {
    m_uniformBuffers.reserve(m_config.maxFramesInFlight);

    for (uint32_t i = 0; i < m_config.maxFramesInFlight; ++i) {
        BufferConfig uboConfig;
        uboConfig.size = sizeof(glm::mat4) * 3; // model, view, proj
        uboConfig.usage = vk::BufferUsageFlagBits::eUniformBuffer;
        uboConfig.memoryProperties =
            vk::MemoryPropertyFlagBits::eHostVisible |
            vk::MemoryPropertyFlagBits::eHostCoherent;

        auto buffer = m_device->CreateBuffer(uboConfig);
        if (!buffer) {
            TE_LOG_ERROR("Failed to create uniform buffer {}", i);
            return false;
        }
        m_uniformBuffers.push_back(std::move(buffer));
    }

    return true;
}

bool VulkanRenderer::CreateDescriptorSets() {
    // 创建描述符池
    std::vector<DescriptorPoolSize> poolSizes;
    poolSizes.push_back({vk::DescriptorType::eUniformBuffer, m_config.maxFramesInFlight});
    poolSizes.push_back({vk::DescriptorType::eCombinedImageSampler, m_config.maxFramesInFlight});

    m_descriptorPool = m_device->CreateDescriptorPool(m_config.maxFramesInFlight * 2, poolSizes);
    if (!m_descriptorPool) {
        return false;
    }

    // 分配 UBO 描述符集
    std::vector<vk::DescriptorSetLayout> uboLayouts(
        m_config.maxFramesInFlight,
        *m_uboDescriptorSetLayout->GetHandle()
    );
    m_uboDescriptorSets = m_descriptorPool->AllocateDescriptorSets(uboLayouts);

    // 分配采样器描述符集
    std::vector<vk::DescriptorSetLayout> samplerLayouts(
        m_config.maxFramesInFlight,
        *m_samplerDescriptorSetLayout->GetHandle()
    );
    m_samplerDescriptorSets = m_descriptorPool->AllocateDescriptorSets(samplerLayouts);

    // 更新描述符集（UBO）
    for (uint32_t i = 0; i < m_config.maxFramesInFlight; ++i) {
        vk::DescriptorBufferInfo bufferInfo;
        bufferInfo.setBuffer(m_uniformBuffers[i]->GetHandle())
                  .setOffset(0)
                  .setRange(sizeof(glm::mat4) * 3);

        vk::WriteDescriptorSet descriptorWrite;
        descriptorWrite.setDstSet(*m_uboDescriptorSets[i])
                       .setDstBinding(0)
                       .setDstArrayElement(0)
                       .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                       .setDescriptorCount(1)
                       .setBufferInfo({bufferInfo});

        m_device->GetHandle().updateDescriptorSets({descriptorWrite}, {});
    }

    return true;
}

bool VulkanRenderer::BeginFrame() {
    // 等待上一帧完成
    auto waitResult = m_device->GetHandle().waitForFences(
        *m_inFlightFences[m_currentFrame],
        true,
        UINT64_MAX
    );
    if (waitResult != vk::Result::eSuccess) {
        TE_LOG_ERROR("Failed to wait for fence: {}", vk::to_string(waitResult));
        return false;
    }

    // 检查是否需要重建交换链
    if (m_framebufferResized) {
        RecreateSwapChain();
        return false; // 重建后需要重新获取图像
    }

    // 如果窗口最小化，跳过
    if (m_window->GetWidth() == 0 || m_window->GetHeight() == 0) {
        return false;
    }

    // 获取下一个可用图像
    auto acquireResult = m_swapChain->AcquireNextImage(
        *m_imageAvailableSemaphores[m_currentFrame],
        nullptr
    );

    if (acquireResult.result == vk::Result::eErrorOutOfDateKHR) {
        RecreateSwapChain();
        return false;
    } else if (acquireResult.result != vk::Result::eSuccess &&
               acquireResult.result != vk::Result::eSuboptimalKHR) {
        TE_LOG_ERROR("Failed to acquire swap chain image: {}", vk::to_string(acquireResult.result));
        return false;
    }

    m_acquiredImageIndex = acquireResult.imageIndex;

    // 检查这个图像是否正在被使用
    if (m_imagesInFlight[m_acquiredImageIndex] != nullptr) {
        m_device->GetHandle().waitForFences(
            *m_imagesInFlight[m_acquiredImageIndex],
            true,
            UINT64_MAX
        );
    }
    m_imagesInFlight[m_acquiredImageIndex] = &m_inFlightFences[m_currentFrame];

    // 重置围栏
    m_device->GetHandle().resetFences(*m_inFlightFences[m_currentFrame]);

    return true;
}

void VulkanRenderer::EndFrame() {
    // 提交命令缓冲区
    m_graphicsQueue->Submit(
        {m_commandBuffers[m_currentFrame]->GetRawHandle()},
        {*m_imageAvailableSemaphores[m_currentFrame]},
        {vk::PipelineStageFlagBits::eColorAttachmentOutput},
        {*m_renderFinishedSemaphores[m_acquiredImageIndex]},
        *m_inFlightFences[m_currentFrame]
    );

    // 呈现图像
    vk::Result presentResult = m_swapChain->Present(
        m_acquiredImageIndex,
        *m_presentQueue,
        {*m_renderFinishedSemaphores[m_acquiredImageIndex]}
    );

    if (presentResult == vk::Result::eErrorOutOfDateKHR ||
        presentResult == vk::Result::eSuboptimalKHR) {
        m_framebufferResized = true;
    } else if (presentResult != vk::Result::eSuccess) {
        TE_LOG_ERROR("Failed to present swap chain image: {}", vk::to_string(presentResult));
    }

    m_currentFrame = (m_currentFrame + 1) % m_config.maxFramesInFlight;
}

void VulkanRenderer::UpdateUniformBuffer(const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj) {
    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    } ubo;

    ubo.model = model;
    ubo.view = view;
    ubo.proj = proj;
    // Vulkan Y 轴翻转
    ubo.proj[1][1] *= -1;

    m_uniformBuffers[m_currentFrame]->UploadData(&ubo, sizeof(ubo));
}

void VulkanRenderer::DrawIndexed(VulkanBuffer* vertexBuffer, VulkanBuffer* indexBuffer, uint32_t indexCount) {
    auto recording = m_commandBuffers[m_currentFrame]->BeginRecording(
        vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    );

    vk::ClearValue clearColor;
    clearColor.setColor(vk::ClearColorValue(std::array<float, 4>{
        m_config.clearColorR,
        m_config.clearColorG,
        m_config.clearColorB,
        m_config.clearColorA
    }));

    {
        auto renderPassScope = m_commandBuffers[m_currentFrame]->BeginRenderPass(
            *m_renderPass,
            *m_framebuffers[m_acquiredImageIndex],
            {clearColor}
        );

        m_commandBuffers[m_currentFrame]->BindPipeline(*m_pipeline);
        m_commandBuffers[m_currentFrame]->BindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_pipeline->GetLayout().operator*(),
            0,
            {*m_uboDescriptorSets[m_currentFrame], *m_samplerDescriptorSets[m_currentFrame]}
        );
        m_commandBuffers[m_currentFrame]->BindVertexBuffer(0, *vertexBuffer, 0);
        m_commandBuffers[m_currentFrame]->BindIndexBuffer(*indexBuffer);

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChain->GetExtent().width);
        viewport.height = static_cast<float>(m_swapChain->GetExtent().height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        m_commandBuffers[m_currentFrame]->SetViewport(viewport);

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = m_swapChain->GetExtent();
        m_commandBuffers[m_currentFrame]->SetScissor(scissor);

        m_commandBuffers[m_currentFrame]->DrawIndexed(indexCount);
    }
}

void VulkanRenderer::OnWindowResize(uint32_t width, uint32_t height) {
    m_framebufferResized = true;
}

void VulkanRenderer::RecreateSwapChain() {
    // 等待设备空闲
    m_device->WaitIdle();

    CleanupSwapChain();

    // 重新创建
    CreateSwapChain();
    CreateFramebuffers();
    CreateCommandBuffers();

    uint32_t imageCount = m_swapChain->GetImageCount();
    m_renderFinishedSemaphores.clear();
    m_renderFinishedSemaphores.reserve(imageCount);
    m_imagesInFlight.resize(imageCount, nullptr);

    for (uint32_t i = 0; i < imageCount; ++i) {
        m_renderFinishedSemaphores.push_back(m_device->CreateVulkanSemaphore());
    }

    m_framebufferResized = false;
}

void VulkanRenderer::CleanupSwapChain() {
    m_framebuffers.clear();
    m_commandBuffers.clear();
    m_swapChain.reset();
}

} // namespace TE