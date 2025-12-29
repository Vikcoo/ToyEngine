// Vulkan Device - 逻辑设备和资源工厂
#pragma once

#include "VulkanPhysicalDevice.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

#include "VulkanImage.h"
#include "VulkanTexture2D.h"


namespace TE {

class VulkanQueue;
class VulkanCommandPool;
class VulkanSwapChain;
class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanSurface;
class VulkanPipeline;
class VulkanBuffer;
class VulkanDescriptorSetLayout;
class VulkanDescriptorPool;
class DescriptorPoolSize;

// 前向声明配置结构（定义在对应的头文件中）
struct SwapChainConfig;
struct AttachmentConfig;
struct GraphicsPipelineConfig;
struct BufferConfig;
struct DescriptorSetLayoutBinding;

/// 设备配置
struct DeviceConfig {
    vk::PhysicalDeviceFeatures enabledFeatures{};
    std::vector<const char*> extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};

/// Vulkan 逻辑设备 - 资源工厂
class VulkanDevice : public std::enable_shared_from_this<VulkanDevice> {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    // 工厂方法
    [[nodiscard]] static std::shared_ptr<VulkanDevice> Create(
        std::shared_ptr<VulkanPhysicalDevice> physicalDevice,
        const QueueFamilyIndices& queueFamilies,
        const DeviceConfig& config = {}
    );

    explicit VulkanDevice(PrivateTag,
                         std::shared_ptr<VulkanPhysicalDevice> physicalDevice,
                         const QueueFamilyIndices& queueFamilies,
                         [[maybe_unused]] const DeviceConfig& config);

    ~VulkanDevice();

    // 禁用拷贝和移动
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) = delete;
    VulkanDevice& operator=(VulkanDevice&&) = delete;

    // 获取队列
    [[nodiscard]] std::shared_ptr<VulkanQueue> GetGraphicsQueue() const { return m_graphicsQueue; }
    [[nodiscard]] std::shared_ptr<VulkanQueue> GetPresentQueue() const { return m_presentQueue; }
    [[nodiscard]] std::shared_ptr<VulkanQueue> GetComputeQueue() const { return m_computeQueue; }
    [[nodiscard]] std::shared_ptr<VulkanQueue> GetTransferQueue() const { return m_transferQueue; }

    // 创建资源
    [[nodiscard]] std::unique_ptr<VulkanCommandPool> CreateCommandPool(
        uint32_t queueFamilyIndex,
        vk::CommandPoolCreateFlags flags = {}
    );

    [[nodiscard]] std::unique_ptr<VulkanSwapChain> CreateSwapChain(
        std::shared_ptr<VulkanSurface> surface,
        const SwapChainConfig& config,
        uint32_t desiredWidth = 1280,
        uint32_t desiredHeight = 720
    );

    [[nodiscard]] std::unique_ptr<VulkanRenderPass> CreateRenderPass(
        const std::vector<AttachmentConfig>& attachments
    );

    [[nodiscard]] std::unique_ptr<VulkanFramebuffer> CreateFramebuffer(
        const VulkanRenderPass& renderPass,
        const std::vector<vk::ImageView>& attachments,
        vk::Extent2D extent
    );

    [[nodiscard]] std::unique_ptr<VulkanPipeline> CreateGraphicsPipeline(
        const VulkanRenderPass& renderPass,
        const GraphicsPipelineConfig& config
    );

    // 创建缓冲区
    [[nodiscard]] std::unique_ptr<VulkanBuffer> CreateBuffer(
        const BufferConfig& config
    );

    // 使用 Staging Buffer 上传数据到设备本地缓冲区（显存）
    // 这是一个辅助方法，用于将 CPU 数据上传到 GPU 显存
    // deviceBuffer: 目标设备本地缓冲区（必须是 eDeviceLocal 内存）
    // data: 要上传的数据
    // size: 数据大小（字节）
    void UploadToDeviceLocalBuffer(
        VulkanBuffer& deviceBuffer,
        const void* data,
        size_t size
    );

    // 创建描述符集布局
    [[nodiscard]] std::unique_ptr<VulkanDescriptorSetLayout> CreateDescriptorSetLayout(
        const std::vector<DescriptorSetLayoutBinding>& bindings
    );

    // 创建描述符池
    [[nodiscard]] std::unique_ptr<VulkanDescriptorPool> CreateDescriptorPool(
        uint32_t maxSets,
        const std::vector<DescriptorPoolSize>& poolSizes,
        vk::DescriptorPoolCreateFlags flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet
    );

    // 创建同步对象
    [[nodiscard]] vk::raii::Semaphore CreateVulkanSemaphore() const;
    [[nodiscard]] vk::raii::Fence CreateFence(bool signaled = true) const;

    // 创建纹理
    [[nodiscard]] std::unique_ptr<VulkanTexture2D> CreateTexture2DFromfile(const std::string& filePath);

    // 创建图像
    [[nodiscard]] std::unique_ptr<VulkanImage> CreateImage(const VulkanImageConfig& config);

    // 等待设备空闲
    void WaitIdle();

    // 获取句柄和信息
    [[nodiscard]] const vk::raii::Device& GetHandle() const { return m_device; }
    [[nodiscard]] const VulkanPhysicalDevice& GetPhysicalDevice() const { return *m_physicalDevice; }
    [[nodiscard]] const vk::raii::PipelineCache& GetPipelineCache() const { return m_pipelineCache; }
    [[nodiscard]] const QueueFamilyIndices& GetQueueFamilies() const { return m_queueFamilies; }

private:
    bool Initialize(const DeviceConfig& config);
    void CreateLogicalDevice(const DeviceConfig& config);
    void CreateQueues();
    void CreatePipelineCache();

private:
    std::shared_ptr<VulkanPhysicalDevice> m_physicalDevice;
    QueueFamilyIndices m_queueFamilies;

    vk::raii::Device m_device{nullptr};
    vk::raii::PipelineCache m_pipelineCache{nullptr};

    std::shared_ptr<VulkanQueue> m_graphicsQueue;
    std::shared_ptr<VulkanQueue> m_presentQueue;
    std::shared_ptr<VulkanQueue> m_computeQueue;
    std::shared_ptr<VulkanQueue> m_transferQueue;
};

} // namespace TE

