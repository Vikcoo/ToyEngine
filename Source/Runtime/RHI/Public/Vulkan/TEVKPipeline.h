//
// Created by yukai on 2025/8/19.
//
#pragma once
#include "TEVKCommon.h"



namespace TE {
    class TEVKLogicDevice;
    class TEVKRenderPass;

    struct ShaderLayout {
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::PushConstantRange> pushConstantRanges;
    };
    struct PipelineVertexInputState {
        std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
    };
    struct PipelineInputAssemblyState {
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        vk::Bool32 primitiveRestartEnable = vk::False;
    };
    struct PipelineRasterizationState {
        vk::Bool32 depthClampEnable = vk::False;
        vk::Bool32 rasterizerDiscardEnable = vk::False;
        vk::PolygonMode polygonMode = vk::PolygonMode::eFill;
        vk::CullModeFlags cullMode = vk::CullModeFlagBits::eNone;
        vk::FrontFace frontFace = vk::FrontFace::eClockwise;
        vk::Bool32 depthBiasEnable = vk::False;
        float depthBiasConstantFactor = 0.0f;
        float depthBiasClamp = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float lineWidth = 1.0f;
    };
    struct PipelineMultisampleState {
        vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;
        vk::Bool32 sampleShadingEnable = vk::False;
        float minSampleShading = 0.2f;
    };
    struct PipelineDepthStencilState {
        vk::Bool32 depthTestEnable = vk::False;
        vk::Bool32 depthWriteEnable = vk::False;
        vk::CompareOp depthCompareOp = vk::CompareOp::eNever;
        vk::Bool32 depthBoundsTestEnable = vk::False;
        vk::Bool32 stencilTestEnable = vk::False;
    };
    struct PipelineDynamicState {
        std::vector<vk::DynamicState> dynamicStates;
    };

    struct PipelineConfig {
        PipelineVertexInputState vertexInputState;
        PipelineInputAssemblyState inputAssemblyState;
        PipelineRasterizationState rasterizationState;
        PipelineMultisampleState multisampleState;
        PipelineDepthStencilState depthStencilState;
        PipelineDynamicState dynamicState;
        vk::PipelineColorBlendAttachmentState colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState()
        .setBlendEnable(vk::False)
        .setSrcColorBlendFactor(vk::BlendFactor::eOne)
        .setDstColorBlendFactor(vk::BlendFactor::eZero)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        .setAlphaBlendOp(vk::BlendOp::eAdd)
        .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    };

    class TEVKPipelineLayout {
    public:
        TEVKPipelineLayout(TEVKLogicDevice& logicDevice, const std::string& vertexShaderFile, const std::string& fragmentShaderFile, const ShaderLayout& layout = {});
        [[nodiscard]] vk::PipelineLayout GetHandle() const {return m_handle;}
        [[nodiscard]] vk::ShaderModule GetVertexShaderModule() const { return m_vertexShaderModule;}
        [[nodiscard]] vk::ShaderModule GetFragmentShaderModule() const { return m_fragmentShaderModule;}
    private:
        vk::Result CreateShaderModule(const std::string& shaderFilePath, vk::raii::ShaderModule& shaderModule);

        vk::raii::PipelineLayout m_handle{VK_NULL_HANDLE};
        vk::raii::ShaderModule m_vertexShaderModule{VK_NULL_HANDLE};
        vk::raii::ShaderModule m_fragmentShaderModule{VK_NULL_HANDLE};
        TEVKLogicDevice& m_logicDevice;
    };

    class TEVKPipeline {
        public:
        TEVKPipeline(TEVKLogicDevice& logicDevice, TEVKRenderPass& renderPass,TEVKPipelineLayout& pipelineLayout);

        TEVKPipeline* SetVertexInputState(const std::vector<vk::VertexInputBindingDescription>& vertexBindings,const std::vector<vk::VertexInputAttributeDescription>& vertexAttrs);
        TEVKPipeline* SetInputAssemblyState(vk::PrimitiveTopology topology, vk::Bool32 primitiveRestartEnable = vk::False);
        TEVKPipeline *SetRasterizationState(const PipelineRasterizationState &rasterizationState);
        TEVKPipeline *SetMultisampleState(vk::SampleCountFlagBits samples,vk::Bool32 sampleShadingEnable, float minSampleShading =0.f);
        TEVKPipeline *SetDepthStencilState(const PipelineDepthStencilState &depthStencilState);
        TEVKPipeline *SetColorBlendAttachmentState(VkBool32 blendEnable,
            vk::BlendFactor srcColorBlendFactor = vk::BlendFactor::eOne, vk::BlendFactor dstColorBlendFactor = vk::BlendFactor::eZero, vk::BlendOp colorBlendOp=vk::BlendOp::eAdd,
            vk::BlendFactor srcAlphaBlendFactor = vk::BlendFactor::eOne, vk::BlendFactor dstAlphaBlendFactor = vk::BlendFactor::eZero, vk::BlendOp alphaBlendOp=vk::BlendOp::eAdd);
        TEVKPipeline *SetDynamicState(const std::vector<vk::DynamicState> &dynamicStates);
        TEVKPipeline *EnableAlphaBlend();
        TEVKPipeline *EnableDepthTest();

        void Create();

        void Bind(vk::raii::CommandBuffer& commandBuffer);

        [[nodiscard]] const vk::raii::Pipeline& GetHandle() const {return m_handle;}
        private:
        vk::raii::Pipeline m_handle{VK_NULL_HANDLE};
        TEVKLogicDevice& m_logicDevice;
        TEVKRenderPass& m_renderPass;
        TEVKPipelineLayout& m_pipelineLayout;

        PipelineConfig m_pipelineConfig;
    };
}
