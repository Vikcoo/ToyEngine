// ToyEngine RHI Module
// RHI 类型定义 - 图形 API 无关的枚举和描述符结构体
// 设计参考 Vulkan/D3D12 的现代图形 API 模型

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace TE {

// 前向声明
class RHIShader;
class RHIBuffer;
class RHIPipeline;
class RHICommandBuffer;
class RHIDevice;
class RHITexture;
class RHISampler;
class RHIBindGroup;
class RHIBindGroupLayout;
class RHIPipelineLayout;

using RHIPlatformPresentCallback = void(*)(void* userData);

// ============================================================
// 枚举类型
// ============================================================

/// 着色器阶段
enum class RHIShaderStage : uint8_t
{
    Vertex,
    Fragment,
    Geometry,       // 预留
    TessControl,    // 预留
    TessEvaluation, // 预留
    Compute,        // 预留
};

/// 缓冲区用途
enum class RHIBufferUsage : uint16_t
{
    None = 0,
    Vertex = 1u << 0u,
    Index = 1u << 1u,
    Uniform = 1u << 2u,
    Storage = 1u << 3u,
    CopySource = 1u << 4u,
    CopyDestination = 1u << 5u,
    Staging = 1u << 6u,
};

[[nodiscard]] constexpr RHIBufferUsage operator|(const RHIBufferUsage lhs, const RHIBufferUsage rhs)
{
    return static_cast<RHIBufferUsage>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
}

[[nodiscard]] constexpr bool HasAnyFlags(const RHIBufferUsage value, const RHIBufferUsage flags)
{
    return (static_cast<uint16_t>(value) & static_cast<uint16_t>(flags)) != 0;
}

enum class RHIMemoryUsage : uint8_t
{
    GPUOnly,
    CPUToGPU,
    GPUToCPU,
};

/// 数据格式（用于顶点属性、纹理格式等）
enum class RHIFormat : uint8_t
{
    Undefined = 0,

    // 浮点格式
    Float,          // 1x float (R32_FLOAT)
    Float2,         // 2x float (R32G32_FLOAT)
    Float3,         // 3x float (R32G32B32_FLOAT)
    Float4,         // 4x float (R32G32B32A32_FLOAT)

    // 整数格式
    Int,            // 1x int32
    Int2,           // 2x int32
    Int3,           // 3x int32
    Int4,           // 4x int32

    // 无符号整数格式
    UInt,           // 1x uint32
    UInt2,          // 2x uint32
    UInt3,          // 3x uint32
    UInt4,          // 4x uint32

    // 归一化格式
    R8_UNorm,
    RG8_UNorm,
    RGB8_UNorm,
    RGBA8_UNorm,
    BGRA8_UNorm,
    RGBA32_Float,

    // sRGB 格式（纹理采样时自动做 sRGB -> Linear 转换）
    RGB8_sRGB,
    RGBA8_sRGB,
    BGRA8_sRGB,

    // 深度/模板格式
    D16_UNorm,
    D24_UNorm_S8_UInt,
    D32_Float,
};

/// 图元拓扑类型
enum class RHIPrimitiveTopology : uint8_t
{
    PointList,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
};

/// 填充模式
enum class RHIPolygonMode : uint8_t
{
    Fill,
    Line,
    Point,
};

/// 面剔除模式
enum class RHICullMode : uint8_t
{
    None,
    Front,
    Back,
    FrontAndBack,
};

/// 正面定义
enum class RHIFrontFace : uint8_t
{
    CounterClockwise,
    Clockwise,
};

/// 比较操作（深度/模板测试）
enum class RHICompareOp : uint8_t
{
    Never,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,
};

/// 索引类型
enum class RHIIndexType : uint8_t
{
    UInt16,
    UInt32,
};

enum class RHISampleCount : uint8_t
{
    Count1 = 1,
    Count2 = 2,
    Count4 = 4,
    Count8 = 8,
};

enum class RHIResourceState : uint8_t
{
    Undefined,
    CopySource,
    CopyDestination,
    VertexBuffer,
    IndexBuffer,
    UniformBuffer,
    RenderTarget,
    DepthWrite,
    DepthRead,
    ShaderResource,
    Present,
};

// ============================================================
// 描述符结构体
// ============================================================

/// 获取 RHIFormat 的字节大小
inline uint32_t GetFormatSize(RHIFormat format)
{
    switch (format)
    {
        case RHIFormat::Float:  return 4;
        case RHIFormat::Float2: return 8;
        case RHIFormat::Float3: return 12;
        case RHIFormat::Float4: return 16;
        case RHIFormat::Int:    return 4;
        case RHIFormat::Int2:   return 8;
        case RHIFormat::Int3:   return 12;
        case RHIFormat::Int4:   return 16;
        case RHIFormat::UInt:   return 4;
        case RHIFormat::UInt2:  return 8;
        case RHIFormat::UInt3:  return 12;
        case RHIFormat::UInt4:  return 16;
        case RHIFormat::R8_UNorm:    return 1;
        case RHIFormat::RG8_UNorm:   return 2;
        case RHIFormat::RGB8_UNorm:  return 3;
        case RHIFormat::RGBA8_UNorm: return 4;
        case RHIFormat::BGRA8_UNorm: return 4;
        case RHIFormat::RGB8_sRGB:   return 3;
        case RHIFormat::RGBA8_sRGB:  return 4;
        case RHIFormat::BGRA8_sRGB:  return 4;
        case RHIFormat::RGBA32_Float: return 16;
        case RHIFormat::D16_UNorm:   return 2;
        case RHIFormat::D24_UNorm_S8_UInt: return 4;
        case RHIFormat::D32_Float:   return 4;
        default: return 0;
    }
}

/// 顶点属性描述（单个属性）
/// 对应 Vulkan 的 VkVertexInputAttributeDescription
struct RHIVertexAttribute
{
    uint32_t    location = 0;       // Shader 中的 location
    RHIFormat   format = RHIFormat::Float3;  // 数据格式
    uint32_t    offset = 0;         // 在顶点结构体中的字节偏移
};

/// 顶点绑定描述（一个 VBO 的布局）
/// 对应 Vulkan 的 VkVertexInputBindingDescription
struct RHIVertexBindingDesc
{
    uint32_t    binding = 0;        // 绑定点索引
    uint32_t    stride = 0;         // 每个顶点的字节步长
};

/// 顶点输入描述（完整的顶点布局）
/// 对应 Vulkan 的 VkPipelineVertexInputStateCreateInfo
struct RHIVertexInputDesc
{
    std::vector<RHIVertexBindingDesc>   bindings;
    std::vector<RHIVertexAttribute>     attributes;
};

/// 光栅化状态描述
/// 对应 Vulkan 的 VkPipelineRasterizationStateCreateInfo
struct RHIRasterizationDesc
{
    RHIPolygonMode  polygonMode = RHIPolygonMode::Fill;
    RHICullMode     cullMode = RHICullMode::Back;
    RHIFrontFace    frontFace = RHIFrontFace::CounterClockwise;
    bool            depthClampEnable = false;
    float           lineWidth = 1.0f;
};

/// 深度模板状态描述
struct RHIDepthStencilDesc
{
    bool            depthTestEnable = true;
    bool            depthWriteEnable = true;
    RHICompareOp    depthCompareOp = RHICompareOp::Less;
};

enum class RHIBlendFactor : uint8_t
{
    Zero,
    One,
    SourceColor,
    OneMinusSourceColor,
    SourceAlpha,
    OneMinusSourceAlpha,
    DestinationColor,
    OneMinusDestinationColor,
    DestinationAlpha,
    OneMinusDestinationAlpha,
};

enum class RHIBlendOp : uint8_t
{
    Add,
    Subtract,
    ReverseSubtract,
    Min,
    Max,
};

enum class RHIColorWriteMask : uint8_t
{
    None = 0,
    Red = 1u << 0u,
    Green = 1u << 1u,
    Blue = 1u << 2u,
    Alpha = 1u << 3u,
    All = 0x0Fu,
};

[[nodiscard]] constexpr bool HasAnyFlags(const RHIColorWriteMask value, const RHIColorWriteMask flags)
{
    return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flags)) != 0;
}

struct RHIColorBlendAttachmentDesc
{
    bool blendEnable = false;
    RHIBlendFactor sourceColorFactor = RHIBlendFactor::One;
    RHIBlendFactor destinationColorFactor = RHIBlendFactor::Zero;
    RHIBlendOp colorBlendOp = RHIBlendOp::Add;
    RHIBlendFactor sourceAlphaFactor = RHIBlendFactor::One;
    RHIBlendFactor destinationAlphaFactor = RHIBlendFactor::Zero;
    RHIBlendOp alphaBlendOp = RHIBlendOp::Add;
    RHIColorWriteMask writeMask = RHIColorWriteMask::All;
};

struct RHIPipelineRenderingDesc
{
    std::vector<RHIFormat> colorAttachmentFormats;
    RHIFormat depthStencilFormat = RHIFormat::Undefined;
    RHISampleCount sampleCount = RHISampleCount::Count1;
    std::vector<RHIColorBlendAttachmentDesc> colorBlendAttachments;
};

/// 缓冲区创建描述符
struct RHIBufferDesc
{
    RHIBufferUsage  usage = RHIBufferUsage::Vertex;
    RHIMemoryUsage  memoryUsage = RHIMemoryUsage::GPUOnly;
    uint64_t        size = 0;               // 缓冲区大小（字节）
    const void*     initialData = nullptr;  // 初始数据（可选）
    std::string     debugName;              // 调试名称（可选）
};

/// 着色器创建描述符
/// Renderer 仅提交逻辑 Shader 名称，不感知具体后端资产路径或字节码格式。
/// 各 RHI 后端负责将 logicalName 解析为 GLSL、SPIR-V、DXIL 等原生资产。
struct RHIShaderDesc
{
    RHIShaderStage stage = RHIShaderStage::Vertex;
    std::string    logicalName;             // 例如 "StaticMesh/BasePassVS"
    std::string    entryPoint = "main";
    std::string    debugName;
};

/// RHI 后端创建时所需的平台桥接信息。
/// RHI 不依赖 Platform 模块；原生窗口句柄及 OpenGL 呈现回调由外层装配提供。
struct RHIDeviceCreateDesc
{
    void* nativeWindowHandle = nullptr;
    void* platformUserData = nullptr;
    RHIPlatformPresentCallback platformPresent = nullptr;
    bool vsync = true;
    uint32_t framesInFlight = 2;
    uint64_t transientUniformBytesPerFrame = 4ull * 1024ull * 1024ull;
};

enum class RHIFrameStatus : uint8_t
{
    Ready,
    Skipped,
    OutOfDate,
    DeviceLost,
    Error,
};

struct RHIFrameBeginInfo
{
    uint32_t framebufferWidth = 0;
    uint32_t framebufferHeight = 0;
    bool vsync = true;
};

/// BeginFrame 返回的短生命周期帧上下文，仅在配对的 EndFrame 前有效。
struct RHIFrameContext
{
    RHICommandBuffer* commandBuffer = nullptr;
    uint64_t frameNumber = 0;
    uint32_t frameIndex = 0;
    uint32_t swapChainImageIndex = 0;
};

struct RHITransientUniformAllocation
{
    RHIBuffer* buffer = nullptr;
    uint64_t offset = 0;
    uint64_t size = 0;
};

/// 图形管线创建描述符
/// 对应 Vulkan 的 VkGraphicsPipelineCreateInfo（简化版）
struct RHIPipelineDesc
{
    RHIShader*              vertexShader = nullptr;
    RHIShader*              fragmentShader = nullptr;
    RHIPipelineLayout*      layout = nullptr;
    RHIVertexInputDesc      vertexInput;
    RHIPrimitiveTopology    topology = RHIPrimitiveTopology::TriangleList;
    RHIRasterizationDesc    rasterization;
    RHIDepthStencilDesc     depthStencil;
    RHIPipelineRenderingDesc rendering;
    std::string             debugName;      // 调试名称（可选）
};

/// 视口
struct RHIViewport
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

/// 裁剪矩形
struct RHIScissorRect
{
    int32_t     x = 0;
    int32_t     y = 0;
    uint32_t    width = 0;
    uint32_t    height = 0;
};

class RHIRenderTarget;

/// 渲染通道开始信息
/// 对应 Vulkan 的 VkRenderPassBeginInfo（简化版）
struct RHIRenderPassBeginInfo
{
    enum class LoadOp : uint8_t
    {
        Load,
        Clear,
        DontCare,
    };

    enum class StoreOp : uint8_t
    {
        Store,
        DontCare,
    };

    float       clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float       clearDepth = 1.0f;
    uint32_t    clearStencil = 0;
    RHIViewport viewport;

    /// 渲染目标（nullptr 表示渲染到默认帧缓冲/交换链）
    RHIRenderTarget* renderTarget = nullptr;
    LoadOp colorLoadOp = LoadOp::Clear;
    StoreOp colorStoreOp = StoreOp::Store;
    LoadOp depthLoadOp = LoadOp::Clear;
    StoreOp depthStoreOp = StoreOp::Store;
};

/// 纹理过滤模式
enum class RHITextureFilter : uint8_t
{
    Nearest,
    Linear,
    LinearMipmapLinear,
};

/// 纹理寻址模式
enum class RHITextureAddressMode : uint8_t
{
    Repeat,
    ClampToEdge,
};

enum class RHITextureDimension : uint8_t
{
    Texture2D,
    TextureCube,
};

enum class RHITextureUsage : uint16_t
{
    None = 0,
    ShaderResource = 1u << 0u,
    ColorAttachment = 1u << 1u,
    DepthStencilAttachment = 1u << 2u,
    CopySource = 1u << 3u,
    CopyDestination = 1u << 4u,
    Storage = 1u << 5u,
};

[[nodiscard]] constexpr RHITextureUsage operator|(const RHITextureUsage lhs, const RHITextureUsage rhs)
{
    return static_cast<RHITextureUsage>(static_cast<uint16_t>(lhs) | static_cast<uint16_t>(rhs));
}

[[nodiscard]] constexpr bool HasAnyFlags(const RHITextureUsage value, const RHITextureUsage flags)
{
    return (static_cast<uint16_t>(value) & static_cast<uint16_t>(flags)) != 0;
}

/// 2D 纹理创建描述符
struct RHITextureDesc
{
    RHITextureDimension dimension = RHITextureDimension::Texture2D;
    uint32_t    width = 0;
    uint32_t    height = 0;
    RHIFormat   format = RHIFormat::RGBA8_UNorm;
    RHITextureUsage usage = RHITextureUsage::ShaderResource | RHITextureUsage::CopyDestination;
    uint32_t    mipLevels = 0; // 0 表示按尺寸推导完整 mip 链
    uint32_t    arrayLayers = 1;
    RHISampleCount sampleCount = RHISampleCount::Count1;
    /// Texture2D 时指向单张图；TextureCube 时指向 +X/-X/+Y/-Y/+Z/-Z 六面连续数据。
    const void* initialData = nullptr;
    uint64_t    initialDataSize = 0; // 0 表示紧密排列并由格式/尺寸推导
    uint32_t    initialDataRowPitch = 0; // 0 表示紧密排列
    bool        generateMips = true;
    bool        srgb = true;
    RHIResourceState initialState = RHIResourceState::Undefined;
    std::string debugName;
};

struct RHITextureBarrier
{
    RHITexture* texture = nullptr;
    RHIResourceState before = RHIResourceState::Undefined;
    RHIResourceState after = RHIResourceState::Undefined;
};

/// 采样器创建描述符
struct RHISamplerDesc
{
    RHITextureFilter     minFilter = RHITextureFilter::Linear;
    RHITextureFilter     magFilter = RHITextureFilter::Linear;
    RHITextureAddressMode addressU = RHITextureAddressMode::Repeat;
    RHITextureAddressMode addressV = RHITextureAddressMode::Repeat;
    RHITextureAddressMode addressW = RHITextureAddressMode::Repeat;
    bool enableAnisotropy = false;
    float maxAnisotropy = 1.0f;
    std::string           debugName;
};

// ============================================================
// 资源绑定描述（BindGroup 模型）
// ============================================================

/// BindGroup 中单个绑定的类型
enum class RHIBindingType : uint8_t
{
    UniformBuffer,  // Vulkan UBO / D3D12 CBV / OpenGL UBO
    DynamicUniformBuffer, // Vulkan dynamic UBO / OpenGL glBindBufferRange
    Texture2D,      // Vulkan Combined Image Sampler / D3D12 SRV / OpenGL texture unit
    TextureCube,    // Cubemap SRV / OpenGL samplerCube
    Sampler,        // 独立采样器（当前简化为与 Texture2D 配对）
};

/// 描述 BindGroup 中单条绑定槽的布局
struct RHIBindGroupLayoutEntry
{
    uint32_t        binding = 0;       // 绑定点索引（对应 Shader 中的 binding）
    RHIBindingType  type = RHIBindingType::UniformBuffer;
    RHIShaderStage  visibility = RHIShaderStage::Vertex; // 对哪些着色器阶段可见（简化版，单阶段）
};

/// BindGroup 布局描述
struct RHIBindGroupLayoutDesc
{
    std::vector<RHIBindGroupLayoutEntry> entries;
    std::string debugName;
};

/// BindGroup 中单条绑定的资源引用
struct RHIBindGroupEntry
{
    uint32_t        binding = 0;
    RHIBindingType  type = RHIBindingType::UniformBuffer;
    RHIBuffer*      buffer = nullptr;        // UniformBuffer 类型时使用
    uint64_t        bufferOffset = 0;
    uint64_t        bufferSize = 0;          // 0 表示整个 buffer
    RHITexture*     texture = nullptr;       // Texture2D 类型时使用
    RHISampler*     sampler = nullptr;       // Sampler 或 Texture2D 配对采样器
};

/// BindGroup 创建描述
struct RHIBindGroupDesc
{
    RHIBindGroupLayout* layout = nullptr;
    std::vector<RHIBindGroupEntry> entries;
    std::string debugName;
};

/// Pipeline 中单个 BindGroupLayout 的声明。
struct RHIPipelineBindGroupLayout
{
    uint32_t groupIndex = 0;
    RHIBindGroupLayout* layout = nullptr;
};

/// Pipeline Layout 描述：声明一个 Pipeline 接受哪些 BindGroup Layout。
/// Vulkan 后端可映射为 VkPipelineLayout，D3D12 后端可映射为 Root Signature。
struct RHIPipelineLayoutDesc
{
    std::vector<RHIPipelineBindGroupLayout> bindGroupLayouts;
    std::string debugName;
};

// ============================================================
// 后端特征描述
// ============================================================

/// 图形 API 后端枚举
enum class ERHIBackendType : uint8_t
{
    OpenGL,
    Vulkan,
    D3D12,
};

/// 后端特征描述，用于上层在需要差异适配时查询后端能力，而非使用 #ifdef。
///
/// 引擎规范约定（所有后端必须在 RHI 层适配到此约定）：
///   - 坐标系：右手 Y-up
///   - NDC 深度：[0, 1]
///   - NDC Y 轴：向上
///   - 纹理原点：左上角（与图片文件存储顺序一致）
///   - 矩阵主序：列主序
///   - 正面缠绕：逆时针
struct RHIBackendTraits
{
    ERHIBackendType backendType = ERHIBackendType::OpenGL;

    // 原生 NDC 深度范围是否为 [0,1]（Vulkan/D3D12 为 true，OpenGL 默认 false）
    bool bNativeNDCDepthZeroToOne = false;

    // 原生 NDC Y 轴是否向上（OpenGL/D3D12 为 true，Vulkan 为 false）
    bool bNativeNDCYAxisUp = true;

    // 原生纹理原点是否在左上角（Vulkan/D3D12 为 true，OpenGL 为 false）
    bool bNativeTextureOriginTopLeft = false;

    // 是否支持 glClipControl（仅 OpenGL 4.5+ 或 GL_ARB_clip_control）
    bool bSupportsClipControl = false;

    // GPU 渲染到 RT 后再作为纹理采样时，是否需要在 shader 中翻转 V。
    bool bRTSampleRequiresFlipY = false;

    /** 当前后端阶段是否已具备完整 Scene Renderer 资源与绘制能力。 */
    bool bSupportsSceneRendering = true;

    /** 是否已具备应用场景、完整 Forward/Deferred 与 PBR 资源能力。 */
    bool bSupportsFullSceneRendering = true;
};

} // namespace TE
