//
// Created by yukai on 2025/8/19.
//
#include "Vulkan/TEVKPipeline.h"
#include "../../Public/TEFileUtil.h"
#include "Vulkan/TEVKLogicDevice.h"
#include "Vulkan/TEVKRenderPass.h"

namespace TE {
    TEVKPipelineLayout::TEVKPipelineLayout(TEVKLogicDevice &logicDevice, const std::string &vertexShaderFile,
        const std::string &fragmentShaderFile, const ShaderLayout &layout):m_logicDevice(logicDevice) {
        // 编译shader compile shader
        CALL_VK_CHECK(CreateShaderModule(vertexShaderFile + ".spv", m_vertexShaderModule));
        CALL_VK_CHECK(CreateShaderModule(fragmentShaderFile + ".spv", m_fragmentShaderModule));
        // create layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo;
        // pipelineLayoutInfo.setSetLayoutCount(layout.descriptorSetLayouts.size());
        // pipelineLayoutInfo.setPSetLayouts(layout.descriptorSetLayouts.data());
        // pipelineLayoutInfo.setPushConstantRangeCount(layout.pushConstantRanges.size());
        // pipelineLayoutInfo.setPushConstantRanges(layout.pushConstantRanges);
        m_handle = m_logicDevice.GetHandle()->createPipelineLayout(pipelineLayoutInfo);
    }

    vk::Result TEVKPipelineLayout::CreateShaderModule(const std::string &shaderFilePath, vk::raii::ShaderModule &shaderModule) {
        std::vector<char> content = ReadCharArrayFromFile(shaderFilePath);

        vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
        shaderModuleCreateInfo.setCodeSize(content.size());
        shaderModuleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(content.data());

        shaderModule = m_logicDevice.GetHandle()->createShaderModule(shaderModuleCreateInfo);
        return shaderModule == VK_NULL_HANDLE ? vk::Result::eErrorUnknown : vk::Result::eSuccess;
    }

    // pipeline
    TEVKPipeline::TEVKPipeline(TEVKLogicDevice &logicDevice, TEVKRenderPass &renderPass,
        TEVKPipelineLayout &pipelineLayout): m_logicDevice(logicDevice), m_renderPass(renderPass), m_pipelineLayout(pipelineLayout) {
    }

    TEVKPipeline * TEVKPipeline::SetVertexInputState(
        const std::vector<vk::VertexInputBindingDescription> &vertexBindings,
        const std::vector<vk::VertexInputAttributeDescription> &vertexAttrs) {
        m_pipelineConfig.vertexInputState.vertexBindingDescriptions = vertexBindings;
        m_pipelineConfig.vertexInputState.vertexAttributeDescriptions = vertexAttrs;
        return this;
    }

    TEVKPipeline * TEVKPipeline::SetInputAssemblyState(vk::PrimitiveTopology topology,
        vk::Bool32 primitiveRestartEnable) {
        m_pipelineConfig.inputAssemblyState.topology = topology;
        m_pipelineConfig.inputAssemblyState.primitiveRestartEnable = primitiveRestartEnable;
        return this;
    }

    TEVKPipeline * TEVKPipeline::SetRasterizationState(const PipelineRasterizationState &rasterizationState) {
        // m_pipelineConfig.rasterizationState.depthClampEnable = rasterizationState.depthClampEnable;
        //        // m_pipelineConfig.rasterizationState.rasterizerDiscardEnable = rasterizationState.rasterizerDiscardEnable;
        //        // m_pipelineConfig.rasterizationState.polygonMode = rasterizationState.polygonMode;
        //        // m_pipelineConfig.rasterizationState.cullMode = rasterizationState.cullMode;
        //        // m_pipelineConfig.rasterizationState.frontFace = rasterizationState.frontFace;
        //        // m_pipelineConfig.rasterizationState.depthBiasEnable = rasterizationState.depthBiasEnable;
        //        // m_pipelineConfig.rasterizationState.depthBiasConstantFactor = rasterizationState.depthBiasConstantFactor;
        //        // m_pipelineConfig.rasterizationState.depthBiasClamp = rasterizationState.depthBiasClamp;
        //        // m_pipelineConfig.rasterizationState.depthBiasSlopeFactor = rasterizationState.depthBiasSlopeFactor;
        //        // m_pipelineConfig.rasterizationState.lineWidth = rasterizationState.lineWidth;
        m_pipelineConfig.rasterizationState = rasterizationState;
        return this;
    }

    TEVKPipeline *TEVKPipeline::SetMultisampleState(vk::SampleCountFlagBits samples, vk::Bool32 sampleShadingEnable,float minSampleShading) {
        m_pipelineConfig.multisampleState.rasterizationSamples = samples;
        m_pipelineConfig.multisampleState.minSampleShading = minSampleShading;
        m_pipelineConfig.multisampleState.sampleShadingEnable = sampleShadingEnable;
        return this;
    }

    TEVKPipeline *TEVKPipeline::SetDepthStencilState(const PipelineDepthStencilState &depthStencilState) {
        m_pipelineConfig.depthStencilState = depthStencilState;
        return this;
    }

    TEVKPipeline *TEVKPipeline::SetColorBlendAttachmentState(VkBool32 blendEnable, vk::BlendFactor srcColorBlendFactor,
                                                             vk::BlendFactor dstColorBlendFactor,
                                                             vk::BlendOp colorBlendOp,
                                                             vk::BlendFactor srcAlphaBlendFactor,
                                                             vk::BlendFactor dstAlphaBlendFactor,
                                                             vk::BlendOp alphaBlendOp) {
        m_pipelineConfig.colorBlendAttachmentState.blendEnable = blendEnable;
        m_pipelineConfig.colorBlendAttachmentState.srcColorBlendFactor = srcColorBlendFactor;
        m_pipelineConfig.colorBlendAttachmentState.dstColorBlendFactor = dstColorBlendFactor;
        m_pipelineConfig.colorBlendAttachmentState.colorBlendOp = colorBlendOp;
        m_pipelineConfig.colorBlendAttachmentState.srcAlphaBlendFactor = srcAlphaBlendFactor;
        m_pipelineConfig.colorBlendAttachmentState.dstAlphaBlendFactor = dstAlphaBlendFactor;
        m_pipelineConfig.colorBlendAttachmentState.alphaBlendOp = alphaBlendOp;
        return this;
    }

    TEVKPipeline *TEVKPipeline::SetDynamicState(const std::vector<vk::DynamicState> &dynamicStates) {
        m_pipelineConfig.dynamicState.dynamicStates = dynamicStates;
        return this;
    }

    TEVKPipeline *TEVKPipeline::EnableAlphaBlend() {
        m_pipelineConfig.colorBlendAttachmentState
        .setBlendEnable(vk::True)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcColor)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd);
        return this;
    }

    TEVKPipeline *TEVKPipeline::EnableDepthTest() {
        m_pipelineConfig.depthStencilState.depthTestEnable = VK_TRUE;
        m_pipelineConfig.depthStencilState.depthWriteEnable = VK_TRUE;
        m_pipelineConfig.depthStencilState.depthCompareOp = vk::CompareOp::eLess;
        return this;
    }

    void TEVKPipeline::Create() {
        std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
        shaderStages.push_back(
            vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(m_pipelineLayout.GetVertexShaderModule())
            .setPName("main")
            );
        shaderStages.push_back(
            vk::PipelineShaderStageCreateInfo()
            .setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(m_pipelineLayout.GetFragmentShaderModule())
            .setPName("main")
            );

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vertexInputInfo.setVertexBindingDescriptions(m_pipelineConfig.vertexInputState.vertexBindingDescriptions);
        vertexInputInfo.setVertexAttributeDescriptions(m_pipelineConfig.vertexInputState.vertexAttributeDescriptions);

        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.setTopology(m_pipelineConfig.inputAssemblyState.topology);
        inputAssembly.setPrimitiveRestartEnable(m_pipelineConfig.inputAssemblyState.primitiveRestartEnable);

        vk::Viewport viewport;
        viewport.setX(0).setY(0).setWidth(100).setHeight(100).setMinDepth(0.0f).setMaxDepth(1.0f);
        vk::Rect2D scissor;
        scissor.setExtent({100,100}).setOffset({0,0});
        vk::PipelineViewportStateCreateInfo viewportState;
        viewportState.setViewportCount(1);
        viewportState.setViewports(viewport);
        viewportState.setScissorCount(1);
        viewportState.setScissors(scissor);

        vk::PipelineRasterizationStateCreateInfo rasterizationState;
        rasterizationState.setDepthClampEnable(m_pipelineConfig.rasterizationState.depthClampEnable);
        rasterizationState.setRasterizerDiscardEnable(m_pipelineConfig.rasterizationState.rasterizerDiscardEnable);
        rasterizationState.setPolygonMode(m_pipelineConfig.rasterizationState.polygonMode);
        rasterizationState.setCullMode(m_pipelineConfig.rasterizationState.cullMode);
        rasterizationState.setFrontFace(m_pipelineConfig.rasterizationState.frontFace);
        rasterizationState.setDepthBiasEnable(m_pipelineConfig.rasterizationState.depthBiasEnable);
        rasterizationState.setDepthBiasConstantFactor(m_pipelineConfig.rasterizationState.depthBiasConstantFactor);
        rasterizationState.setDepthBiasClamp(m_pipelineConfig.rasterizationState.depthBiasClamp);
        rasterizationState.setDepthBiasSlopeFactor(m_pipelineConfig.rasterizationState.depthBiasSlopeFactor);
        rasterizationState.setLineWidth(m_pipelineConfig.rasterizationState.lineWidth);
        // 测试
        rasterizationState.polygonMode = vk::PolygonMode::eFill;
        rasterizationState.lineWidth = 1.0f;
        rasterizationState.cullMode = vk::CullModeFlagBits::eBack;
        rasterizationState.frontFace = vk::FrontFace::eClockwise;

        vk::PipelineMultisampleStateCreateInfo multisampleState;
        multisampleState.setRasterizationSamples(m_pipelineConfig.multisampleState.rasterizationSamples);
        multisampleState.setSampleShadingEnable(m_pipelineConfig.multisampleState.sampleShadingEnable);
        multisampleState.setMinSampleShading(m_pipelineConfig.multisampleState.minSampleShading);
        multisampleState.setPSampleMask(nullptr);
        multisampleState.setAlphaToCoverageEnable(vk::False);
        multisampleState.setAlphaToOneEnable(vk::False);


        vk::PipelineDepthStencilStateCreateInfo depthStencilState;
        depthStencilState.setDepthTestEnable(m_pipelineConfig.depthStencilState.depthTestEnable);
        depthStencilState.setDepthWriteEnable(m_pipelineConfig.depthStencilState.depthWriteEnable);
        depthStencilState.setDepthCompareOp(m_pipelineConfig.depthStencilState.depthCompareOp);
        depthStencilState.setDepthBoundsTestEnable(m_pipelineConfig.depthStencilState.depthBoundsTestEnable);
        depthStencilState.setStencilTestEnable(m_pipelineConfig.depthStencilState.stencilTestEnable);
        depthStencilState.setFront({});
        depthStencilState.setBack({});
        depthStencilState.setMinDepthBounds(0);
        depthStencilState.setMaxDepthBounds(0);

        vk::PipelineColorBlendStateCreateInfo colorBlendState;
        colorBlendState.setLogicOpEnable(vk::False);
        colorBlendState.setLogicOp(vk::LogicOp::eClear);
        colorBlendState.setAttachmentCount(1);
        colorBlendState.setPAttachments(&m_pipelineConfig.colorBlendAttachmentState);
        colorBlendState.setBlendConstants(std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f});

        vk::PipelineDynamicStateCreateInfo dynamicState;
        dynamicState.setDynamicStates(m_pipelineConfig.dynamicState.dynamicStates);

        vk::GraphicsPipelineCreateInfo graphicsPipelineCreateInfo;
        graphicsPipelineCreateInfo.setStages(shaderStages);
        graphicsPipelineCreateInfo.setPVertexInputState(&vertexInputInfo);
        graphicsPipelineCreateInfo.setPInputAssemblyState(&inputAssembly);
        graphicsPipelineCreateInfo.setPViewportState(&viewportState);
        graphicsPipelineCreateInfo.setPRasterizationState(&rasterizationState);
        graphicsPipelineCreateInfo.setPMultisampleState(&multisampleState);
        graphicsPipelineCreateInfo.setPDepthStencilState(&depthStencilState);
        graphicsPipelineCreateInfo.setPColorBlendState(&colorBlendState);
        graphicsPipelineCreateInfo.setPDynamicState(&dynamicState);
        graphicsPipelineCreateInfo.setLayout(m_pipelineLayout.GetHandle());
        graphicsPipelineCreateInfo.setRenderPass(m_renderPass.GetHandle());
        graphicsPipelineCreateInfo.setSubpass(0);
        graphicsPipelineCreateInfo.setBasePipelineHandle(VK_NULL_HANDLE);
        graphicsPipelineCreateInfo.setBasePipelineIndex(0);
        graphicsPipelineCreateInfo.setLayout(m_pipelineLayout.GetHandle());

        m_handle = m_logicDevice.GetHandle()->createGraphicsPipeline(m_logicDevice.GetPipelineCache(),graphicsPipelineCreateInfo);
        LOG_TRACE("Pipeline created: {}", reinterpret_cast<uint64_t>(static_cast<void*>(*m_handle)));
    }

    void TEVKPipeline::Bind(vk::raii::CommandBuffer &commandBuffer) {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_handle);
    }
}
