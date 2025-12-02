// Vulkan Command Buffer 实现
#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "Log/Log.h"

namespace TE {

// ============================================================================
// CommandBufferRecordingScope
// ============================================================================

CommandBufferRecordingScope::CommandBufferRecordingScope(
    vk::raii::CommandBuffer& commandBuffer,
    const vk::CommandBufferUsageFlags flags)
    : m_commandBuffer(commandBuffer)
{
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(flags);
    
    try {
        m_commandBuffer.begin(beginInfo);
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to begin command buffer recording: {}", e.what());
        throw;
    }
}

CommandBufferRecordingScope::~CommandBufferRecordingScope() {
    try {
        m_commandBuffer.end();
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to end command buffer recording: {}", e.what());
    }
}

// ============================================================================
// RenderPassScope
// ============================================================================

RenderPassScope::RenderPassScope(
    vk::raii::CommandBuffer& commandBuffer,
    const VulkanRenderPass& renderPass,
    const VulkanFramebuffer& framebuffer,
    const std::vector<vk::ClearValue>& clearValues)
    : m_commandBuffer(commandBuffer)
{
    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.setRenderPass(*renderPass.GetHandle())
                  .setFramebuffer(*framebuffer.GetHandle())
                  .setRenderArea(vk::Rect2D({0, 0}, framebuffer.GetExtent()))
                  .setClearValues(clearValues);
    
    m_commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

RenderPassScope::~RenderPassScope() {
    try {
        m_commandBuffer.endRenderPass();
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to end render pass: {}", e.what());
    }
}

// ============================================================================
// VulkanCommandBuffer
// ============================================================================

VulkanCommandBuffer::VulkanCommandBuffer(PrivateTag,
                                         std::shared_ptr<VulkanDevice> device,
                                         vk::raii::CommandBuffer commandBuffer)
    : m_device(std::move(device))
    , m_commandBuffer(std::move(commandBuffer))
{
    TE_LOG_DEBUG("Command buffer created");
}

CommandBufferRecordingScope VulkanCommandBuffer::BeginRecording(
    const vk::CommandBufferUsageFlags flags)
{
    return CommandBufferRecordingScope(m_commandBuffer, flags);
}

RenderPassScope VulkanCommandBuffer::BeginRenderPass(
    const VulkanRenderPass& renderPass,
    const VulkanFramebuffer& framebuffer,
    const std::vector<vk::ClearValue>& clearValues)
{
    return RenderPassScope(m_commandBuffer, renderPass, framebuffer, clearValues);
}

void VulkanCommandBuffer::BindPipeline(const VulkanPipeline& pipeline) {
    m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline.GetHandle());
}

void VulkanCommandBuffer::SetViewport(const vk::Viewport& viewport) {
    m_commandBuffer.setViewport(0, {viewport});
}

void VulkanCommandBuffer::SetScissor(const vk::Rect2D& scissor) {
    m_commandBuffer.setScissor(0, {scissor});
}

void VulkanCommandBuffer::Draw(const uint32_t vertexCount,
                               const uint32_t instanceCount,
                               const uint32_t firstVertex,
                               const uint32_t firstInstance) {
    m_commandBuffer.draw(vertexCount, instanceCount, firstVertex, firstInstance);
}

void VulkanCommandBuffer::Begin(const vk::CommandBufferUsageFlags flags) {
    vk::CommandBufferBeginInfo beginInfo;
    beginInfo.setFlags(flags);
    m_commandBuffer.begin(beginInfo);
}

void VulkanCommandBuffer::End() {
    m_commandBuffer.end();
}

void VulkanCommandBuffer::BeginRenderPassInternal(
    const VulkanRenderPass& renderPass,
    const VulkanFramebuffer& framebuffer,
    const std::vector<vk::ClearValue>& clearValues)
{
    vk::RenderPassBeginInfo renderPassInfo;
    renderPassInfo.setRenderPass(*renderPass.GetHandle())
                  .setFramebuffer(*framebuffer.GetHandle())
                  .setRenderArea(vk::Rect2D({0, 0}, framebuffer.GetExtent()))
                  .setClearValues(clearValues);
    
    m_commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
}

void VulkanCommandBuffer::EndRenderPassInternal() {
    m_commandBuffer.endRenderPass();
}

} // namespace TE
