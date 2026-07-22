// ToyEngine RHIVulkan Module
// Vulkan CommandBuffer 阶段 B 实现

#include "VulkanCommandBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanDescriptors.h"
#include "VulkanPipeline.h"
#include "VulkanTexture.h"
#include "Log/Log.h"

#include <algorithm>

namespace TE {

namespace {

VkAttachmentLoadOp ToVulkanLoadOp(const RHIRenderPassBeginInfo::LoadOp loadOp)
{
    switch (loadOp)
    {
    case RHIRenderPassBeginInfo::LoadOp::Load: return VK_ATTACHMENT_LOAD_OP_LOAD;
    case RHIRenderPassBeginInfo::LoadOp::Clear: return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case RHIRenderPassBeginInfo::LoadOp::DontCare: return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp ToVulkanStoreOp(const RHIRenderPassBeginInfo::StoreOp storeOp)
{
    return storeOp == RHIRenderPassBeginInfo::StoreOp::Store
        ? VK_ATTACHMENT_STORE_OP_STORE
        : VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

} // namespace

VulkanCommandBuffer::VulkanCommandBuffer(const VkCommandBuffer commandBuffer)
    : m_CommandBuffer(commandBuffer)
{
}

void VulkanCommandBuffer::PrepareSwapchainTarget(const VkImageView imageView,
                                                 const VkImageView depthImageView,
                                                 const VkExtent2D extent)
{
    m_SwapchainImageView = imageView;
    m_SwapchainDepthImageView = depthImageView;
    m_SwapchainExtent = extent;
    m_BoundPipeline = nullptr;
    m_Executable = false;
}

void VulkanCommandBuffer::Begin()
{
    if (m_CommandBuffer == VK_NULL_HANDLE || m_Recording)
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid CommandBuffer Begin");
        return;
    }

    const VkCommandBufferBeginInfo beginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    const VkResult result = vkBeginCommandBuffer(m_CommandBuffer, &beginInfo);
    if (result != VK_SUCCESS)
    {
        TE_LOG_ERROR("[RHIVulkan] vkBeginCommandBuffer failed: {}", static_cast<int32_t>(result));
        return;
    }

    m_Recording = true;
    m_Executable = false;
}

void VulkanCommandBuffer::BeginRenderPass(const RHIRenderPassBeginInfo& info)
{
    if (!m_Recording || m_Rendering || m_SwapchainImageView == VK_NULL_HANDLE)
    {
        TE_LOG_ERROR("[RHIVulkan] BeginRenderPass called outside a valid frame");
        return;
    }
    if (info.renderTarget)
    {
        TE_LOG_ERROR("[RHIVulkan] Stage B only supports the swapchain render target");
        return;
    }

    VkRenderingAttachmentInfo colorAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_SwapchainImageView,
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = ToVulkanLoadOp(info.colorLoadOp),
        .storeOp = ToVulkanStoreOp(info.colorStoreOp),
    };
    colorAttachment.clearValue.color.float32[0] = info.clearColor[0];
    colorAttachment.clearValue.color.float32[1] = info.clearColor[1];
    colorAttachment.clearValue.color.float32[2] = info.clearColor[2];
    colorAttachment.clearValue.color.float32[3] = info.clearColor[3];

    VkRenderingAttachmentInfo depthAttachment{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_SwapchainDepthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .loadOp = ToVulkanLoadOp(info.depthLoadOp),
        .storeOp = ToVulkanStoreOp(info.depthStoreOp),
    };
    depthAttachment.clearValue.depthStencil = {info.clearDepth, info.clearStencil};

    const VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {{0, 0}, m_SwapchainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = m_SwapchainDepthImageView != VK_NULL_HANDLE ? &depthAttachment : nullptr,
    };
    vkCmdBeginRendering(m_CommandBuffer, &renderingInfo);
    m_Rendering = true;

    const RHIViewport viewport = info.viewport.width > 0.0f && info.viewport.height > 0.0f
        ? info.viewport
        : RHIViewport{0.0f, 0.0f,
                      static_cast<float>(m_SwapchainExtent.width),
                      static_cast<float>(m_SwapchainExtent.height), 0.0f, 1.0f};
    SetViewport(viewport);
    SetScissor({0, 0, m_SwapchainExtent.width, m_SwapchainExtent.height});
}

void VulkanCommandBuffer::EndRenderPass()
{
    if (!m_Rendering)
    {
        TE_LOG_ERROR("[RHIVulkan] EndRenderPass called without active Dynamic Rendering");
        return;
    }
    vkCmdEndRendering(m_CommandBuffer);
    m_Rendering = false;
}

void VulkanCommandBuffer::BindPipeline(RHIPipeline* pipeline)
{
    const auto* native = dynamic_cast<VulkanPipeline*>(pipeline);
    if (!m_Recording || !native || !native->IsValid())
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid graphics pipeline binding");
        return;
    }
    vkCmdBindPipeline(m_CommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, native->GetHandle());
    m_BoundPipeline = native;
}

void VulkanCommandBuffer::BindVertexBuffer(RHIBuffer* buffer, uint32_t binding, uint64_t offset)
{
    const auto* native = dynamic_cast<VulkanBuffer*>(buffer);
    if (!m_Recording || !native)
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid vertex buffer binding");
        return;
    }
    const VkBuffer handle = native->GetHandle();
    const VkDeviceSize nativeOffset = offset;
    vkCmdBindVertexBuffers(m_CommandBuffer, binding, 1, &handle, &nativeOffset);
}

void VulkanCommandBuffer::BindIndexBuffer(RHIBuffer* buffer, RHIIndexType indexType, uint64_t offset)
{
    const auto* native = dynamic_cast<VulkanBuffer*>(buffer);
    if (!m_Recording || !native)
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid index buffer binding");
        return;
    }
    vkCmdBindIndexBuffer(m_CommandBuffer, native->GetHandle(), offset,
                         indexType == RHIIndexType::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
}

void VulkanCommandBuffer::SetViewport(const RHIViewport& viewport)
{
    if (!m_Recording)
    {
        return;
    }

    // Vulkan NDC Y 差异由 Device::AdjustProjectionMatrix 统一处理，Viewport 保持正高度。
    const VkViewport nativeViewport{
        .x = viewport.x,
        .y = viewport.y,
        .width = viewport.width,
        .height = viewport.height,
        .minDepth = viewport.minDepth,
        .maxDepth = viewport.maxDepth,
    };
    vkCmdSetViewport(m_CommandBuffer, 0, 1, &nativeViewport);
}

void VulkanCommandBuffer::SetScissor(const RHIScissorRect& scissor)
{
    if (!m_Recording)
    {
        return;
    }

    const VkRect2D nativeScissor{
        .offset = {std::max(scissor.x, 0), std::max(scissor.y, 0)},
        .extent = {scissor.width, scissor.height},
    };
    vkCmdSetScissor(m_CommandBuffer, 0, 1, &nativeScissor);
}

void VulkanCommandBuffer::TransitionTexture(const RHITextureBarrier& barrier)
{
    auto* native = dynamic_cast<VulkanTexture*>(barrier.texture);
    if (!m_Recording || !native)
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid texture transition");
        return;
    }
    native->RecordTransition(m_CommandBuffer, barrier.before, barrier.after);
}

void VulkanCommandBuffer::Draw(uint32_t vertexCount, uint32_t firstVertex,
                               uint32_t instanceCount, uint32_t firstInstance)
{
    if (m_Recording && m_Rendering)
    {
        vkCmdDraw(m_CommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }
}

void VulkanCommandBuffer::DrawIndexed(uint32_t indexCount, uint32_t firstIndex,
                                      int32_t vertexOffset, uint32_t instanceCount,
                                      uint32_t firstInstance)
{
    if (m_Recording && m_Rendering)
    {
        vkCmdDrawIndexed(m_CommandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}

void VulkanCommandBuffer::SetBindGroup(uint32_t groupIndex,
                                       RHIBindGroup* bindGroup,
                                       std::span<const uint32_t> dynamicOffsets)
{
    const auto* nativeGroup = dynamic_cast<VulkanBindGroup*>(bindGroup);
    if (!m_Recording || !m_BoundPipeline || !m_BoundPipeline->GetLayout() || !nativeGroup)
    {
        TE_LOG_ERROR("[RHIVulkan] BindGroup requires a bound compatible pipeline");
        return;
    }
    const VkDescriptorSet descriptorSet = nativeGroup->GetHandle();
    vkCmdBindDescriptorSets(m_CommandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_BoundPipeline->GetLayout()->GetHandle(),
                            groupIndex,
                            1,
                            &descriptorSet,
                            static_cast<uint32_t>(dynamicOffsets.size()),
                            dynamicOffsets.data());
}

void VulkanCommandBuffer::End()
{
    if (!m_Recording)
    {
        TE_LOG_ERROR("[RHIVulkan] Invalid CommandBuffer End");
        return;
    }
    if (m_Rendering)
    {
        TE_LOG_ERROR("[RHIVulkan] CommandBuffer ended with active Dynamic Rendering");
        EndRenderPass();
    }

    const VkResult result = vkEndCommandBuffer(m_CommandBuffer);
    m_Recording = false;
    m_Executable = result == VK_SUCCESS;
    if (!m_Executable)
    {
        TE_LOG_ERROR("[RHIVulkan] vkEndCommandBuffer failed: {}", static_cast<int32_t>(result));
    }
}

} // namespace TE
