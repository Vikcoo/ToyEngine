// ToyEngine RHIVulkan Module
// Vulkan CommandBuffer 阶段 B 实现

#pragma once

#include "RHICommandBuffer.h"

#include <vulkan/vulkan.h>

namespace TE {

class VulkanPipeline;

class VulkanCommandBuffer final : public RHICommandBuffer
{
public:
    explicit VulkanCommandBuffer(VkCommandBuffer commandBuffer);
    ~VulkanCommandBuffer() override = default;

    void PrepareSwapchainTarget(VkImageView imageView, VkImageView depthImageView, VkExtent2D extent);

    void Begin() override;
    void BeginRenderPass(const RHIRenderPassBeginInfo& info) override;
    void EndRenderPass() override;
    void BindPipeline(RHIPipeline* pipeline) override;
    void BindVertexBuffer(RHIBuffer* buffer, uint32_t binding, uint64_t offset) override;
    void BindIndexBuffer(RHIBuffer* buffer, RHIIndexType indexType, uint64_t offset) override;
    void SetViewport(const RHIViewport& viewport) override;
    void SetScissor(const RHIScissorRect& scissor) override;
    void TransitionTexture(const RHITextureBarrier& barrier) override;
    void Draw(uint32_t vertexCount, uint32_t firstVertex,
              uint32_t instanceCount, uint32_t firstInstance) override;
    void DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                     int32_t vertexOffset, uint32_t instanceCount,
                     uint32_t firstInstance) override;
    void SetBindGroup(uint32_t groupIndex,
                      RHIBindGroup* bindGroup,
                      std::span<const uint32_t> dynamicOffsets) override;
    void End() override;

    [[nodiscard]] VkCommandBuffer GetHandle() const { return m_CommandBuffer; }
    [[nodiscard]] bool IsExecutable() const { return m_Executable; }
    [[nodiscard]] bool IsRendering() const { return m_Rendering; }

private:
    VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
    VkImageView m_SwapchainImageView = VK_NULL_HANDLE;
    VkImageView m_SwapchainDepthImageView = VK_NULL_HANDLE;
    VkExtent2D m_SwapchainExtent{};
    const VulkanPipeline* m_BoundPipeline = nullptr;
    bool m_Recording = false;
    bool m_Rendering = false;
    bool m_Executable = false;
};

} // namespace TE
