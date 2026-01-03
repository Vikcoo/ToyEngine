// Vulkan Device 实现
#include "VulkanDevice.h"
#include "VulkanQueue.h"
#include "VulkanCommandPool.h"
#include "VulkanSwapChain.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "VulkanBuffer.h"
#include "VulkanCommandBuffer.h"
#include "VulkanDescriptorSetLayout.h"
#include "VulkanDescriptorPool.h"
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
    auto device = std::make_shared<VulkanDevice>(PrivateTag{}, std::move(physicalDevice), queueFamilies, config);

    if (!device->Initialize(config)) {
        TE_LOG_ERROR("Failed to initialize Vulkan device");
        return nullptr;
    }

    TE_LOG_INFO("Vulkan Logical device create successfully");
    return device;
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

    // 创建队列创建信息 每个队列族取一个队列用
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (const uint32_t queueFamily : uniqueQueueFamilies) {
        constexpr float queuePriority = 1.0f;
        vk::DeviceQueueCreateInfo queueCreateInfo;
        queueCreateInfo.setQueueFamilyIndex(queueFamily);
        queueCreateInfo.setQueueCount(1);   // todo:
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
    if (m_queueFamilies.graphics.has_value()) {\
        // 之前只申请创建一个queue() todo:
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

vk::raii::Semaphore VulkanDevice::CreateVulkanSemaphore() const
{
    vk::SemaphoreCreateInfo createInfo;
    return m_device.createSemaphore(createInfo);
}

vk::raii::Fence VulkanDevice::CreateFence(const bool signaled) const
{
    vk::FenceCreateInfo createInfo;
    if (signaled) {
        createInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    }
    return m_device.createFence(createInfo);
}

std::unique_ptr<VulkanTexture2D> VulkanDevice::CreateTexture2DFromfile(const std::string& filePath)
{
    return VulkanTexture2D::CreateFromFile(shared_from_this(), filePath);
}

std::unique_ptr<VulkanImage> VulkanDevice::CreateImage(const VulkanImageConfig& config)
{
    return VulkanImage::Create(shared_from_this(), config);
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

std::unique_ptr<VulkanBuffer> VulkanDevice::CreateBuffer(const BufferConfig& config)
{
    return std::make_unique<VulkanBuffer>(
        VulkanBuffer::PrivateTag{},
        shared_from_this(),
        config
    );
}

void VulkanDevice::UploadToDeviceLocalBuffer(
    VulkanBuffer& deviceBuffer,
    const void* data,
    size_t size)
{
    if (!data || size == 0) {
        TE_LOG_ERROR("Invalid data or size for UploadToDeviceLocalBuffer");
        return;
    }

    // 1. 创建临时 Staging Buffer（主机可见内存，用于 CPU 上传数据）
    BufferConfig stagingConfig;
    stagingConfig.size = size;
    stagingConfig.usage = vk::BufferUsageFlagBits::eTransferSrc;  // 作为传输源
    stagingConfig.memoryProperties = 
        vk::MemoryPropertyFlagBits::eHostVisible | 
        vk::MemoryPropertyFlagBits::eHostCoherent;  // 主机可见且一致

    auto stagingBuffer = CreateBuffer(stagingConfig);
    if (!stagingBuffer) {
        TE_LOG_ERROR("Failed to create staging buffer");
        return;
    }

    // 2. 上传数据到 Staging Buffer（CPU 直接写入）
    stagingBuffer->UploadData(data, size);
    TE_LOG_DEBUG("Data uploaded to staging buffer: {} bytes", size);

    // 3. 创建临时命令池和命令缓冲区（用于传输操作）
    //    使用图形队列族（大多数设备图形队列也支持传输操作）
    auto transferCommandPool = CreateCommandPool(
        m_queueFamilies.graphics.value(),
        vk::CommandPoolCreateFlagBits::eTransient  // 临时命令池，用完即弃
    );
    if (!transferCommandPool) {
        TE_LOG_ERROR("Failed to create transfer command pool");
        return;
    }

    auto transferCommandBuffers = transferCommandPool->AllocateCommandBuffers(1);
    if (transferCommandBuffers.empty()) {
        TE_LOG_ERROR("Failed to allocate transfer command buffer");
        return;
    }
    auto& transferCmdBuffer = transferCommandBuffers[0];

    // 4. 录制复制命令
    {
        auto recording = transferCmdBuffer->BeginRecording(
            vk::CommandBufferUsageFlagBits::eOneTimeSubmit  // 一次性提交
        );

        // 复制：从 Staging Buffer 到 Device Buffer
        transferCmdBuffer->CopyBuffer(
            *stagingBuffer,
            deviceBuffer,
            size,
            0,  // srcOffset
            0   // dstOffset
        );
    }

    // 5. 提交复制命令到图形队列并等待完成
    //    注意：使用图形队列，因为大多数设备图形队列也支持传输操作
    //    如果有专门的传输队列，可以使用传输队列以获得更好的性能
    auto graphicsQueue = GetGraphicsQueue();
    if (!graphicsQueue) {
        TE_LOG_ERROR("Graphics queue is null");
        return;
    }

    graphicsQueue->Submit(
        {transferCmdBuffer->GetRawHandle()},
        {},  // 无等待信号量
        {},  // 无等待阶段
        {},  // 无信号量
        nullptr  // 无围栏
    );

    // 6. 等待复制完成（阻塞直到 GPU 完成复制）
    graphicsQueue->WaitIdle();
    TE_LOG_DEBUG("Data copied from staging buffer to device local buffer: {} bytes", size);

    // 7. Staging Buffer 和命令缓冲区自动析构（RAII）
    //    注意：命令池也会自动析构，但命令缓冲区会先析构
}

std::unique_ptr<VulkanDescriptorSetLayout> VulkanDevice::CreateDescriptorSetLayout(
    const std::vector<DescriptorSetLayoutBinding>& bindings){
    return std::make_unique<VulkanDescriptorSetLayout>(VulkanDescriptorSetLayout::PrivateTag(), shared_from_this() ,bindings);
}

std::unique_ptr<VulkanDescriptorPool> VulkanDevice::CreateDescriptorPool(uint32_t maxSets,
    const std::vector<DescriptorPoolSize>& poolSizes, vk::DescriptorPoolCreateFlags flags){
    return std::make_unique<VulkanDescriptorPool>(VulkanDescriptorPool::PrivateTag(), shared_from_this(), maxSets, poolSizes, flags);
}
} // namespace TE

