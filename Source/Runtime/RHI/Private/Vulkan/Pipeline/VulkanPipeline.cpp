// Vulkan Pipeline 实现
#include "Pipeline/VulkanPipeline.h"
#include "Core/VulkanDevice.h"
#include "Pipeline/VulkanRenderPass.h"
#include "Pipeline/VulkanShader.h"
#include "Log/Log.h"

namespace TE {

VulkanPipeline::VulkanPipeline(PrivateTag,
                               std::shared_ptr<VulkanDevice> device,
                               const VulkanRenderPass& renderPass,
                               const GraphicsPipelineConfig& config)
    : m_device(std::move(device))
{
    // 创建着色器
    m_vertexShader = std::make_unique<VulkanShader>(
        VulkanShader::PrivateTag{},
        m_device,
        config.vertexShaderPath
    );
    
    m_fragmentShader = std::make_unique<VulkanShader>(
        VulkanShader::PrivateTag{},
        m_device,
        config.fragmentShaderPath
    );
    
    CreatePipelineLayout(config);
    CreatePipeline(renderPass, config);
}

VulkanPipeline::~VulkanPipeline() {
    TE_LOG_DEBUG("Pipeline destroyed");
}

void VulkanPipeline::CreatePipelineLayout(const GraphicsPipelineConfig& config) {
    vk::PipelineLayoutCreateInfo createInfo;
    
    // 设置描述符集布局（如果配置中提供了）
    if (!config.descriptorSetLayouts.empty()) {
        createInfo.setSetLayouts(config.descriptorSetLayouts);
    }
    
    // 可以添加 PushConstants 支持
    // createInfo.setPushConstantRanges(...);
    
    try {
        m_layout = m_device->GetHandle().createPipelineLayout(createInfo);
        TE_LOG_DEBUG("Pipeline layout created with {} descriptor set layout(s)", 
                    config.descriptorSetLayouts.size());
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create pipeline layout: {}", e.what());
        throw;
    }
}

void VulkanPipeline::CreatePipeline(const VulkanRenderPass& renderPass, 
                                    const GraphicsPipelineConfig& config) {
    // 着色器阶段
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    
    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
    vertShaderStageInfo.setModule(*m_vertexShader->GetHandle());
    vertShaderStageInfo.setPName("main");
    shaderStages.push_back(vertShaderStageInfo);
    
    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
    fragShaderStageInfo.setModule(*m_fragmentShader->GetHandle());
    fragShaderStageInfo.setPName("main");
    shaderStages.push_back(fragShaderStageInfo);
    
    // 顶点输入
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.setVertexBindingDescriptions(config.vertexBindings);
    vertexInputInfo.setVertexAttributeDescriptions(config.vertexAttributes);
    
    // 输入组装
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.setTopology(config.topology);
    inputAssembly.setPrimitiveRestartEnable(config.primitiveRestartEnable);
    
    // 视口和剪裁
    // 注意：如果使用了动态状态（eViewport和eScissor），这里的设置会被忽略
    // 但仍然需要提供一个有效的viewport和scissor（即使会被动态状态覆盖）
    vk::PipelineViewportStateCreateInfo viewportState;
    if (!config.dynamicStates.empty() && 
        std::find(config.dynamicStates.begin(), config.dynamicStates.end(), 
                  vk::DynamicState::eViewport) != config.dynamicStates.end() &&
        std::find(config.dynamicStates.begin(), config.dynamicStates.end(), 
                  vk::DynamicState::eScissor) != config.dynamicStates.end()) {
        // 使用动态状态时，仍然需要提供一个viewport和scissor（会被忽略）
        // 但必须提供至少一个viewport和scissor
        viewportState.setViewportCount(1);
        viewportState.setPViewports(&config.viewport);
        viewportState.setScissorCount(1);
        viewportState.setPScissors(&config.scissor);
    } else {
        // 不使用动态状态时，使用配置中的viewport和scissor
        viewportState.setViewports(config.viewport);
        viewportState.setScissors(config.scissor);
    }
    
    // 光栅化
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.setDepthClampEnable(VK_FALSE)
              .setRasterizerDiscardEnable(VK_FALSE)
              .setPolygonMode(config.polygonMode)
              .setLineWidth(config.lineWidth)
              .setCullMode(config.cullMode)
              .setFrontFace(config.frontFace)
              .setDepthBiasEnable(VK_FALSE);
    
    // 多重采样
    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setSampleShadingEnable(VK_FALSE)
                 .setRasterizationSamples(config.rasterizationSamples);
    
    // 深度测试
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(config.depthTestEnable)
                 .setDepthWriteEnable(config.depthWriteEnable)
                 .setDepthCompareOp(config.depthCompareOp)
                 .setDepthBoundsTestEnable(VK_FALSE)
                 .setStencilTestEnable(VK_FALSE);
    
    // 颜色混合
    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.setColorWriteMask(
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA
    )
    .setBlendEnable(config.blendEnable)
    .setSrcColorBlendFactor(config.srcColorBlendFactor)
    .setDstColorBlendFactor(config.dstColorBlendFactor)
    .setColorBlendOp(config.colorBlendOp)
    .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
    .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
    .setAlphaBlendOp(vk::BlendOp::eAdd);
    
    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setLogicOpEnable(VK_FALSE)
                  .setAttachments(colorBlendAttachment);
    
    // 动态状态
    vk::PipelineDynamicStateCreateInfo dynamicState;
    dynamicState.setDynamicStates(config.dynamicStates);
    
    // 创建管线
    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStages(shaderStages)
                .setPVertexInputState(&vertexInputInfo)
                .setPInputAssemblyState(&inputAssembly)
                .setPViewportState(&viewportState)
                .setPRasterizationState(&rasterizer)
                .setPMultisampleState(&multisampling)
                .setPDepthStencilState(&depthStencil)
                .setPColorBlendState(&colorBlending)
                .setPDynamicState(&dynamicState)
                .setLayout(*m_layout)
                .setRenderPass(*renderPass.GetHandle())
                .setSubpass(0)
                .setBasePipelineHandle(nullptr)
                .setBasePipelineIndex(-1);
    
    try {
        auto result = m_device->GetHandle().createGraphicsPipeline(
            m_device->GetPipelineCache(),
            pipelineInfo
        );
        m_pipeline = std::move(result);
        TE_LOG_INFO("Graphics pipeline created: {} + {}", 
                   config.vertexShaderPath, 
                   config.fragmentShaderPath);
    }
    catch (const vk::SystemError& e) {
        TE_LOG_ERROR("Failed to create graphics pipeline: {}", e.what());
        throw;
    }
}

void VulkanPipeline::Bind(vk::raii::CommandBuffer& commandBuffer) const {
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
}

} // namespace TE
