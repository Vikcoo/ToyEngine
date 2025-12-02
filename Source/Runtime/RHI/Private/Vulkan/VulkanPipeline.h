// Vulkan Pipeline - 图形管线
#pragma once

#include "VulkanDevice.h"
#include "VulkanRenderPass.h"
#include "VulkanShader.h"
#include <vulkan/vulkan_raii.hpp>
#include <memory>
#include <vector>

namespace TE {

class VulkanDevice;
class VulkanRenderPass;
class VulkanShader;

/// 图形管线配置
struct GraphicsPipelineConfig {
    // 着色器
    std::string vertexShaderPath;
    std::string fragmentShaderPath;
    
    // 顶点输入
    std::vector<vk::VertexInputBindingDescription> vertexBindings;
    std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
    
    // 输入组装
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    vk::Bool32 primitiveRestartEnable = VK_FALSE;
    
    // 视口和剪裁
    vk::Viewport viewport;
    vk::Rect2D scissor;
    
    // 光栅化
    vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
    vk::FrontFace frontFace = vk::FrontFace::eCounterClockwise;
    float lineWidth = 1.0f;
    
    // 多重采样
    vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;
    
    // 深度测试
    vk::Bool32 depthTestEnable = VK_FALSE;
    vk::Bool32 depthWriteEnable = VK_FALSE;
    vk::CompareOp depthCompareOp = vk::CompareOp::eLess;
    
    // 颜色混合
    vk::Bool32 blendEnable = VK_FALSE;
    vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eOne;
    vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eZero;
    vk::BlendOp colorBlendOp = vk::BlendOp::eAdd;
    
    // 动态状态
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };
};

/// Vulkan 图形管线
class VulkanPipeline {
public:
    // Passkey 模式
    struct PrivateTag {
    private:
        explicit PrivateTag() = default;
        friend class VulkanDevice;
    };

    explicit VulkanPipeline(PrivateTag,
                           std::shared_ptr<VulkanDevice> device,
                           const VulkanRenderPass& renderPass,
                           const GraphicsPipelineConfig& config);

    ~VulkanPipeline();

    // 禁用拷贝，允许移动
    VulkanPipeline(const VulkanPipeline&) = delete;
    VulkanPipeline& operator=(const VulkanPipeline&) = delete;
    VulkanPipeline(VulkanPipeline&&) noexcept = default;
    VulkanPipeline& operator=(VulkanPipeline&&) noexcept = default;

    // 绑定管线到命令缓冲
    void Bind(vk::raii::CommandBuffer& commandBuffer) const;

    // 获取句柄
    [[nodiscard]] const vk::raii::Pipeline& GetHandle() const { return m_pipeline; }
    [[nodiscard]] const vk::raii::PipelineLayout& GetLayout() const { return m_layout; }

private:
    void CreatePipelineLayout();
    void CreatePipeline(const VulkanRenderPass& renderPass, const GraphicsPipelineConfig& config);

private:
    std::shared_ptr<VulkanDevice> m_device;
    vk::raii::PipelineLayout m_layout{nullptr};
    vk::raii::Pipeline m_pipeline{nullptr};
    
    std::unique_ptr<VulkanShader> m_vertexShader;
    std::unique_ptr<VulkanShader> m_fragmentShader;
};

} // namespace TE
