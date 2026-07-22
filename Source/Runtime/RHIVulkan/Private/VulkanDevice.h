// ToyEngine RHIVulkan Module
// Vulkan Device、Swapchain、资源与帧同步阶段 B 实现

#pragma once

#include "RHIDevice.h"
#include "RHITransientAllocator.h"

#include <vulkan/vulkan.h>

#include <functional>
#include <memory>
#include <vector>

namespace TE {

class VulkanCommandBuffer;
class VulkanTexture;

class VulkanDevice final : public RHIDevice
{
public:
    explicit VulkanDevice(const RHIDeviceCreateDesc& desc);
    ~VulkanDevice() override;

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    [[nodiscard]] bool IsValid() const { return m_Valid; }

    [[nodiscard]] VkDevice GetNativeDevice() const { return m_Device; }
    [[nodiscard]] VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPool; }
    [[nodiscard]] bool CreateBufferAllocation(VkDeviceSize size,
                                              VkBufferUsageFlags usage,
                                              VkMemoryPropertyFlags properties,
                                              VkBuffer& outBuffer,
                                              VkDeviceMemory& outMemory) const;
    [[nodiscard]] bool CreateImageAllocation(uint32_t width,
                                             uint32_t height,
                                             VkFormat format,
                                             VkImageUsageFlags usage,
                                             VkSampleCountFlagBits sampleCount,
                                             VkImage& outImage,
                                             VkDeviceMemory& outMemory) const;
    [[nodiscard]] bool ExecuteImmediate(const std::function<void(VkCommandBuffer)>& recorder);
    [[nodiscard]] bool UploadBuffer(VkBuffer destination,
                                    VkDeviceSize destinationOffset,
                                    const void* data,
                                    VkDeviceSize size);
    [[nodiscard]] bool UploadTexture2D(VkImage destination,
                                      uint32_t width,
                                      uint32_t height,
                                      VkImageAspectFlags aspectMask,
                                      const void* data,
                                      uint64_t dataSize,
                                      uint64_t sourceRowPitch,
                                      uint64_t tightRowPitch);

    [[nodiscard]] RHIFrameStatus BeginFrame(const RHIFrameBeginInfo& beginInfo,
                                            RHIFrameContext& outContext) override;
    [[nodiscard]] RHIFrameStatus EndFrame(RHIFrameContext& context) override;
    void WaitIdle() override;
    [[nodiscard]] bool AllocateTransientUniform(const void* data,
                                                uint64_t size,
                                                RHITransientUniformAllocation& outAllocation) override;

    [[nodiscard]] std::unique_ptr<RHIBuffer> CreateBuffer(const RHIBufferDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIShader> CreateShader(const RHIShaderDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIPipeline> CreatePipeline(const RHIPipelineDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHICommandBuffer> CreateCommandBuffer() override;
    [[nodiscard]] std::unique_ptr<RHITexture> CreateTexture(const RHITextureDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHISampler> CreateSampler(const RHISamplerDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIBindGroup> CreateBindGroup(const RHIBindGroupDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIBindGroupLayout> CreateBindGroupLayout(const RHIBindGroupLayoutDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIPipelineLayout> CreatePipelineLayout(const RHIPipelineLayoutDesc& desc) override;
    [[nodiscard]] std::unique_ptr<RHIRenderTarget> CreateRenderTarget(const RHIRenderTargetDesc& desc) override;

    [[nodiscard]] const RHIBackendTraits& GetBackendTraits() const override { return m_Traits; }
    [[nodiscard]] RHIFormat GetBackBufferColorFormat() const override { return m_BackBufferFormat; }
    [[nodiscard]] RHIFormat GetBackBufferDepthFormat() const override { return RHIFormat::D32_Float; }
    [[nodiscard]] Matrix4 AdjustProjectionMatrix(const Matrix4& projection) const override;

private:
    struct FFrameResources
    {
        VkCommandPool CommandPool = VK_NULL_HANDLE;
        VkCommandBuffer NativeCommandBuffer = VK_NULL_HANDLE;
        std::unique_ptr<VulkanCommandBuffer> CommandBuffer;
        VkSemaphore ImageAvailable = VK_NULL_HANDLE;
        VkFence InFlight = VK_NULL_HANDLE;
    };

    [[nodiscard]] bool Initialize(const RHIDeviceCreateDesc& desc);
    [[nodiscard]] bool CreateInstance();
    [[nodiscard]] bool CreateDebugMessenger();
    [[nodiscard]] bool CreateSurface(void* nativeWindowHandle);
    [[nodiscard]] bool SelectPhysicalDevice();
    [[nodiscard]] bool CreateLogicalDevice();
    [[nodiscard]] bool CreateDescriptorPool();
    [[nodiscard]] bool CreateImmediateResources();
    [[nodiscard]] bool CreateFrameResources(uint32_t frameCount);
    [[nodiscard]] RHIFrameStatus CreateSwapchain(uint32_t width, uint32_t height, bool vsync);
    [[nodiscard]] RHIFrameStatus RecreateSwapchain(uint32_t width, uint32_t height, bool vsync);
    void DestroySwapchain();
    void DestroyFrameResources();
    void DestroyDebugMessenger();
    void RecordSwapchainTransition(VkCommandBuffer commandBuffer,
                                   VkImage image,
                                   VkImageLayout oldLayout,
                                   VkImageLayout newLayout) const;

    RHIBackendTraits m_Traits;
    bool m_Valid = false;
    bool m_FrameActive = false;
    bool m_SwapchainDirty = true;
    bool m_ValidationEnabled = false;

    VkInstance m_Instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
    VkDevice m_Device = VK_NULL_HANDLE;
    VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
    VkQueue m_PresentQueue = VK_NULL_HANDLE;
    uint32_t m_GraphicsQueueFamily = UINT32_MAX;
    uint32_t m_PresentQueueFamily = UINT32_MAX;
    VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
    VkCommandPool m_ImmediateCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer m_ImmediateCommandBuffer = VK_NULL_HANDLE;

    VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
    VkFormat m_SwapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_SwapchainExtent{};
    std::vector<VkImage> m_SwapchainImages;
    std::vector<VkImageView> m_SwapchainImageViews;
    std::vector<VkSemaphore> m_RenderFinishedSemaphores;
    std::vector<std::unique_ptr<VulkanTexture>> m_SwapchainDepthTextures;
    std::vector<VkFence> m_ImagesInFlight;
    std::vector<bool> m_ImageInitialized;
    RHIFormat m_BackBufferFormat = RHIFormat::Undefined;
    uint32_t m_LastRequestedWidth = 0;
    uint32_t m_LastRequestedHeight = 0;
    bool m_VSync = true;

    std::vector<FFrameResources> m_Frames;
    std::unique_ptr<RHIBuffer> m_TransientUniformBuffer;
    RHITransientRangeAllocator m_TransientUniformAllocator;
    uint32_t m_CurrentFrame = 0;
    uint32_t m_ActiveImageIndex = 0;
    uint64_t m_FrameNumber = 0;
};

} // namespace TE
