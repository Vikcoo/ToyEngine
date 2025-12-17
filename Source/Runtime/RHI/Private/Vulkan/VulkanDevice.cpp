// Vulkan Device 实现
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandPool.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "VulkanUtils.h"
#include "Log/Log.h"
#include <set>

namespace TE {

// ============================================================================
// 工厂方法和构造/析构
// ============================================================================

std::shared_ptr<VulkanDevice> VulkanDevice::Create(
    std::shared_ptr<VulkanPhysicalDevice> physicalDevice,
    const QueueFamilyIndices& queueFamilies,
    const DeviceConfig& config)
{
    auto device = std::make_shared<VulkanDevice>(
        PrivateTag{},
        std::move(physicalDevice),
        queueFamilies,
        config
    );

    if (!device->Initialize(config)) {
        TE_LOG_ERROR("Failed to initialize Vulkan device");
        return nullptr;
    }

    TE_LOG_INFO("Vulkan Logical device create successfully");
    return device;
}

VulkanDevice::VulkanDevice(PrivateTag,
                           std::shared_ptr<VulkanPhysicalDevice> physicalDevice,
                           const QueueFamilyIndices& queueFamilies,
                           [[maybe_unused]] const DeviceConfig& config)
    : m_physicalDevice(std::move(physicalDevice))
    , m_queueFamilies(queueFamilies)
{
    TE_LOG_INFO("Initializing Vulkan Device");
}

VulkanDevice::~VulkanDevice() {
    if (m_device != nullptr) {
        WaitIdle();
    }
    TE_LOG_INFO("Vulkan Device destroyed");
}

// ============================================================================
// 初始化
// ============================================================================

bool VulkanDevice::Initialize(const DeviceConfig& config) {
    if (!m_queueFamilies.IsComplete()) {
        TE_LOG_ERROR("Queue families incomplete");
        return false;
    }

    // 检查扩展支持
    if (!m_physicalDevice->CheckExtensionSupport(config.extensions)) {
        TE_LOG_ERROR("Required device extensions not supported");
        return false;
    }

    CreateLogicalDevice(config);
    CreateQueues();
    CreatePipelineCache();

    return true;
}

void VulkanDevice::CreateLogicalDevice(const DeviceConfig& config) {
    // 验证启用的特性是否被物理设备支持
    const auto supportedFeatures = m_physicalDevice->GetFeatures();
    vk::PhysicalDeviceFeatures enabledFeatures = config.enabledFeatures;
    
    // 检查每个启用的特性是否被支持（简化检查，只检查常见特性）
    if (enabledFeatures.samplerAnisotropy && !supportedFeatures.samplerAnisotropy) {
        TE_LOG_WARN("Requested samplerAnisotropy feature is not supported, disabling it");
        enabledFeatures.samplerAnisotropy = VK_FALSE;
    }
    if (enabledFeatures.geometryShader && !supportedFeatures.geometryShader) {
        TE_LOG_WARN("Requested geometryShader feature is not supported, disabling it");
        enabledFeatures.geometryShader = VK_FALSE;
    }
    if (enabledFeatures.tessellationShader && !supportedFeatures.tessellationShader) {
        TE_LOG_WARN("Requested tessellationShader feature is not supported, disabling it");
        enabledFeatures.tessellationShader = VK_FALSE;
    }
    
    // 收集所有唯一的队列族索引
    std::set<uint32_t> uniqueQueueFamilies;
    if (m_queueFamilies.graphics.has_value()) {
        uniqueQueueFamilies.insert(m_queueFamilies.graphics.value());
    }
    if (m_queueFamilies.present.has_value()) {
        uniqueQueueFamilies.insert(m_queueFamilies.present.value());
    }
    if (m_queueFamilies.compute.has_value()) {
        uniqueQueueFamilies.insert(m_queueFamilies.compute.value());
    }
    if (m_queueFamilies.transfer.has_value()) {
        uniqueQueueFamilies.insert(m_queueFamilies.transfer.value());
    }

    // 创建队列创建信息
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    const float queuePriority = 1.0f;

    for (const uint32_t queueFamily : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setQueueFamilyIndex(queueFamily);
        queueCreateInfo.setQueueCount(1);
        queueCreateInfo.setQueuePriorities(queuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // 创建设备
    vk::DeviceCreateInfo createInfo;
    createInfo.setQueueCreateInfos(queueCreateInfos)
              .setPEnabledExtensionNames(config.extensions)
              .setPEnabledFeatures(&enabledFeatures);

    m_device = m_physicalDevice->GetHandle().createDevice(createInfo);
    if (m_device == nullptr)
    {
        TE_LOG_ERROR("Failed to create logical device");
    }
    TE_LOG_INFO("Logical device created");
}

void VulkanDevice::CreateQueues() {
    // 创建图形队列
    if (m_queueFamilies.graphics.has_value()) {
        auto queue = m_device.getQueue(m_queueFamilies.graphics.value(), 0);
        m_graphicsQueue = std::make_shared<VulkanQueue>(
            VulkanQueue::PrivateTag{},
            std::move(queue),
            m_queueFamilies.graphics.value(),
            0
        );
        TE_LOG_DEBUG("Graphics queue created");
    }

    // 创建呈现队列
    if (m_queueFamilies.present.has_value()) {
        // 如果与图形队列是同一个，则共享
        if (m_queueFamilies.IsSameGraphicsPresent() && m_graphicsQueue) {
            m_presentQueue = m_graphicsQueue;
            TE_LOG_DEBUG("Present queue shared with graphics queue");
        } else {
            auto queue = m_device.getQueue(m_queueFamilies.present.value(), 0);
            m_presentQueue = std::make_shared<VulkanQueue>(
                VulkanQueue::PrivateTag{},
                std::move(queue),
                m_queueFamilies.present.value(),
                0
            );
            TE_LOG_DEBUG("Present queue created");
        }
    }

    // 创建计算队列
    if (m_queueFamilies.compute.has_value()) {
        auto queue = m_device.getQueue(m_queueFamilies.compute.value(), 0);
        m_computeQueue = std::make_shared<VulkanQueue>(
            VulkanQueue::PrivateTag{},
            std::move(queue),
            m_queueFamilies.compute.value(),
            0
        );
        TE_LOG_DEBUG("Compute queue created");
    }

    // 创建传输队列
    if (m_queueFamilies.transfer.has_value()) {
        auto queue = m_device.getQueue(m_queueFamilies.transfer.value(), 0);
        m_transferQueue = std::make_shared<VulkanQueue>(
            VulkanQueue::PrivateTag{},
            std::move(queue),
            m_queueFamilies.transfer.value(),
            0
        );
        TE_LOG_DEBUG("Transfer queue created");
    }
}

void VulkanDevice::CreatePipelineCache() {
    vk::PipelineCacheCreateInfo createInfo;

    m_pipelineCache = m_device.createPipelineCache(createInfo);
    if (m_pipelineCache == nullptr){
        TE_LOG_ERROR("Failed to create pipeline cache");
    }
    TE_LOG_DEBUG("Pipeline cache created");
}

// ============================================================================
// 资源创建
// ============================================================================

std::unique_ptr<VulkanCommandPool> VulkanDevice::CreateCommandPool(
    const uint32_t queueFamilyIndex,
    const vk::CommandPoolCreateFlags flags)
{
    return std::make_unique<VulkanCommandPool>(
        VulkanCommandPool::PrivateTag{},
        shared_from_this(),
        queueFamilyIndex,
        flags
    );
}

vk::raii::Semaphore VulkanDevice::CreateSemaphore() {
    vk::SemaphoreCreateInfo createInfo;
    return m_device.createSemaphore(createInfo);
}

vk::raii::Fence VulkanDevice::CreateFence(const bool signaled) {
    vk::FenceCreateInfo createInfo;
    if (signaled) {
        createInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    }
    return m_device.createFence(createInfo);
}

// ============================================================================
// 设备操作
// ============================================================================

void VulkanDevice::WaitIdle() {
    try {
        m_device.waitIdle();
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Device waitIdle failed: {}", e.what());
        throw;
    }
}

// ============================================================================
// 资源创建（层次3）
// ============================================================================

std::unique_ptr<VulkanSwapChain> VulkanDevice::CreateSwapChain(
    std::shared_ptr<VulkanSurface> surface,
    const SwapChainConfig& config,
    uint32_t desiredWidth,
    uint32_t desiredHeight)
{
    if (!surface) {
        TE_LOG_ERROR("Cannot create swap chain: surface is null");
        return nullptr;
    }
    
    return std::make_unique<VulkanSwapChain>(
        VulkanSwapChain::PrivateTag{},
        shared_from_this(),
        std::move(surface),
        config,
        desiredWidth,
        desiredHeight
    );
}

std::unique_ptr<VulkanRenderPass> VulkanDevice::CreateRenderPass(
    const std::vector<AttachmentConfig>& attachments)
{
    return std::make_unique<VulkanRenderPass>(
        VulkanRenderPass::PrivateTag{},
        shared_from_this(),
        attachments
    );
}

std::unique_ptr<VulkanFramebuffer> VulkanDevice::CreateFramebuffer(
    const VulkanRenderPass& renderPass,
    const std::vector<vk::ImageView>& attachments,
    const vk::Extent2D extent)
{
    return std::make_unique<VulkanFramebuffer>(
        VulkanFramebuffer::PrivateTag{},
        shared_from_this(),
        renderPass,
        attachments,
        extent
    );
}

std::unique_ptr<VulkanPipeline> VulkanDevice::CreateGraphicsPipeline(
    const VulkanRenderPass& renderPass,
    const GraphicsPipelineConfig& config)
{
    return std::make_unique<VulkanPipeline>(
        VulkanPipeline::PrivateTag{},
        std::const_pointer_cast<VulkanDevice>(shared_from_this()),
        renderPass,
        config
    );
}

} // namespace TE

