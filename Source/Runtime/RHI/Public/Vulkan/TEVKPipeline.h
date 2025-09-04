/*
文件用途: 图形管线与管线布局封装
- 负责: 创建 ShaderModule、PipelineLayout，并以“配置体”的方式组装 Graphics Pipeline
- 概念: Pipeline 描述了固定功能与可编程阶段，Layout 约束了 Descriptor/Push Constant 等接口
*/
#pragma once
#include "TEVKCommon.h"

namespace TE {
    class TEVKLogicDevice;
    class TEVKRenderPass;

    // 着色器布局（可扩展）：描述符集合布局 + Push常量范围
    struct ShaderLayout {
        std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
        std::vector<vk::PushConstantRange> pushConstantRanges;
    };
    // 顶点输入配置
    struct PipelineVertexInputState {
        std::vector<vk::VertexInputBindingDescription> vertexBindingDescriptions;
        std::vector<vk::VertexInputAttributeDescription> vertexAttributeDescriptions;
    };
    // 组装状态（图元拓扑等）
    struct PipelineInputAssemblyState {
        vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
        vk::Bool32 primitiveRestartEnable = vk::False;
    };
    // 光栅化状态
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
    // 多重采样状态
    struct PipelineMultisampleState {
        vk::SampleCountFlagBits rasterizationSamples = vk::SampleCountFlagBits::e1;
        vk::Bool32 sampleShadingEnable = vk::False;
        float minSampleShading = 0.2f;
    };
    // 深度/模板测试状态
    struct PipelineDepthStencilState {
        vk::Bool32 depthTestEnable = vk::False;
        vk::Bool32 depthWriteEnable = vk::False;
        vk::CompareOp depthCompareOp = vk::CompareOp::eNever;
        vk::Bool32 depthBoundsTestEnable = vk::False;
        vk::Bool32 stencilTestEnable = vk::False;
    };
    // 动态状态（如视口/剪裁由命令缓冲动态设置）
    struct PipelineDynamicState {
        std::vector<vk::DynamicState> dynamicStates;
    };

    // 管线配置集合（按阶段归档，便于阅读与复用）
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

    // 管线布局：加载/持有 ShaderModule，创建 PipelineLayout
    class TEVKPipelineLayout {
    public:
        TEVKPipelineLayout(TEVKLogicDevice& logicDevice, const std::string& vertexShaderFile, const std::string& fragmentShaderFile, const ShaderLayout& layout = {});
        [[nodiscard]] vk::PipelineLayout GetHandle() const {return m_handle;}
        [[nodiscard]] vk::ShaderModule GetVertexShaderModule() const { return m_vertexShaderModule;}
        [[nodiscard]] vk::ShaderModule GetFragmentShaderModule() const { return m_fragmentShaderModule;}
    private:
        // 从 .spv 文件创建 ShaderModule
        vk::Result CreateShaderModule(const std::string& shaderFilePath, vk::raii::ShaderModule& shaderModule);

        vk::raii::PipelineLayout m_handle{VK_NULL_HANDLE};
        vk::raii::ShaderModule m_vertexShaderModule{VK_NULL_HANDLE};
        vk::raii::ShaderModule m_fragmentShaderModule{VK_NULL_HANDLE};
        TEVKLogicDevice& m_logicDevice;
    };

    // 图形管线：以“Builder”风格配置各固定功能状态并创建 Pipeline
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

        // 创建 Graphics Pipeline（依赖 RenderPass 与 PipelineLayout）
        void Create();

        // 绑定到命令缓冲，准备绘制
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