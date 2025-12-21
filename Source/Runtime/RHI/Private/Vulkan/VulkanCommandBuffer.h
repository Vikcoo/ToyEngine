// Vulkan Command Buffer - 命令缓冲（RAII Scope 模式）
#pragma once

#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace TE {

class VulkanDevice;
class VulkanRenderPass;
class VulkanFramebuffer;
class VulkanPipeline;
class VulkanBuffer;

/// 命令缓冲录制 Scope - RAII 自动配对 begin/end
class CommandBufferRecordingScope {
public:
    explicit CommandBufferRecordingScope(vk::raii::CommandBuffer& commandBuffer,
                                        vk::CommandBufferUsageFlags flags = {});
    ~CommandBufferRecordingScope();

    // 禁用拷贝
    CommandBufferRecordingScope(const CommandBufferRecordingScope&) = delete;
    CommandBufferRecordingScope& operator=(const CommandBufferRecordingScope&) = delete;

private:
    vk::raii::CommandBuffer& m_commandBuffer;
};

/// 渲染通道 Scope - RAII 自动配对 beginRenderPass/endRenderPass
class RenderPassScope {
public:
    explicit RenderPassScope(vk::raii::CommandBuffer& commandBuffer,
                            const VulkanRenderPass& renderPass,
                            const VulkanFramebuffer& framebuffer,
                            const std::vector<vk::ClearValue>& clearValues);
    ~RenderPassScope();

    // 禁用拷贝
    RenderPassScope(const RenderPassScope&) = delete;
    RenderPassScope& operator=(const RenderPassScope&) = delete;

private:
    vk::raii::CommandBuffer& m_commandBuffer;
};

/// Vulkan 命令缓冲封装
class VulkanCommandBuffer {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanCommandPool;
    };

    explicit VulkanCommandBuffer(PrivateTag,
                                std::shared_ptr<VulkanDevice> device,
                                vk::raii::CommandBuffer commandBuffer);

    // 禁用拷贝，允许移动
    VulkanCommandBuffer(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer& operator=(const VulkanCommandBuffer&) = delete;
    VulkanCommandBuffer(VulkanCommandBuffer&&) noexcept = default;
    VulkanCommandBuffer& operator=(VulkanCommandBuffer&&) noexcept = default;

    // RAII 录制 Scope
    [[nodiscard]] CommandBufferRecordingScope BeginRecording(
        vk::CommandBufferUsageFlags flags = {}
    );

    // RAII 渲染通道 Scope
    [[nodiscard]] RenderPassScope BeginRenderPass(
        const VulkanRenderPass& renderPass,
        const VulkanFramebuffer& framebuffer,
        const std::vector<vk::ClearValue>& clearValues = {}
    );

    // 命令录制接口
    void BindPipeline(const VulkanPipeline& pipeline);
    void BindVertexBuffer(uint32_t firstBinding, 
                         const VulkanBuffer& buffer, 
                         size_t offset = 0);
    void CopyBuffer(const VulkanBuffer& srcBuffer,
                   const VulkanBuffer& dstBuffer,
                   size_t size,
                   size_t srcOffset = 0,
                   size_t dstOffset = 0);
    void SetViewport(const vk::Viewport& viewport);
    void SetScissor(const vk::Rect2D& scissor);
    void Draw(uint32_t vertexCount, 
             uint32_t instanceCount = 1,
             uint32_t firstVertex = 0,
             uint32_t firstInstance = 0);

    // 获取底层句柄
    [[nodiscard]] const vk::raii::CommandBuffer& GetHandle() const { return m_commandBuffer; }
    [[nodiscard]] vk::CommandBuffer GetRawHandle() const { return *m_commandBuffer; }

private:
    void Begin(vk::CommandBufferUsageFlags flags);
    void End();
    void BeginRenderPassInternal(const VulkanRenderPass& renderPass,
                                 const VulkanFramebuffer& framebuffer,
                                 const std::vector<vk::ClearValue>& clearValues);
    void EndRenderPassInternal();

    friend class CommandBufferRecordingScope;
    friend class RenderPassScope;

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::CommandBuffer m_commandBuffer;
};

} // namespace TE
