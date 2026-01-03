//
// Created by yukai on 2026/1/3.
//

#pragma once
#include <cstdint>
#include <memory>

namespace TE
{
    struct RawTextureData;

    // 通用纹理格式（映射Vulkan的VkFormat和DX的DXGI_FORMAT）
    enum class RHITextureFormat {
        R8G8B8A8_UNORM,  // 最常用：RGBA8位无符号归一化
        R8G8B8_UNORM,    // RGB8位无符号归一化
        D32_SFLOAT,      // 32位浮点深度
        D24_UNORM_S8_UINT,// 24位深度+8位模板
        R32G32B32A32_SFLOAT // 32位浮点RGBA
    };

    // 通用纹理状态（映射Vulkan的图像布局和DX12的资源状态）
    enum class RHIResourceState {
        Undefined,               // 未初始化
        ShaderReadOnly,          // 着色器只读（采样使用）
        RenderTarget,            // 渲染目标（颜色附件）
        DepthStencil,            // 深度模板附件
        HostVisible,             // CPU可访问（上传/下载数据）
        CopySource,              // 拷贝源
        CopyDestination          // 拷贝目标
    };

    // 通用纹理采样模式
    enum class RHISamplerFilter {
        Nearest,  // 邻近过滤
        Linear    // 线性过滤
    };

    enum class RHISamplerAddressMode {
        Repeat,        // 重复
        ClampToEdge,   // 夹紧到边缘
        MirrorRepeat   // 镜像重复
    };

    // 采样器抽象接口（与纹理分离，兼容所有API）
    class RHISampler {
    public:
        virtual ~RHISampler() = default;
        // 通用接口：设置采样参数（按需扩展）
        virtual void SetFilter(RHISamplerFilter minFilter, RHISamplerFilter magFilter) = 0;
        virtual void SetAddressMode(RHISamplerAddressMode uMode, RHISamplerAddressMode vMode) = 0;
    };

    // 纹理抽象接口（核心抽象，与API无关）
    class RHITexture {
    public:
        virtual ~RHITexture() = default;

        // 通用接口：获取纹理属性
        virtual uint32_t GetWidth() const = 0;
        virtual uint32_t GetHeight() const = 0;
        virtual RHITextureFormat GetFormat() const = 0;
        virtual uint32_t GetMipLevelCount() const = 0;

        // 通用接口：更新纹理数据（CPU → GPU，与API无关）
        // 参数：原始纹理数据 + 目标mip层级
        virtual bool UpdateData(const RawTextureData& rawData, uint32_t mipLevel = 0) = 0;

        // 通用接口：转换纹理状态（映射Vulkan布局转换/DX12资源屏障）
        virtual void TransitionState(RHIResourceState targetState) = 0;

        // 通用接口：创建采样器（返回抽象采样器）
        virtual std::unique_ptr<RHISampler> CreateSampler() = 0;
    };

    // 纹理工厂抽象（用于创建具体API纹理，工厂模式）
    class RHITextureFactory {
    public:
        virtual ~RHITextureFactory() = default;
        // 核心：从原始纹理数据创建API纹理（与API无关的入口）
        virtual std::unique_ptr<RHITexture> CreateTexture(const RawTextureData& rawData, RHITextureFormat format) = 0;
    };
}
