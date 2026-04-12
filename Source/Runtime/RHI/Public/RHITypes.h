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
enum class RHIBufferUsage : uint8_t
{
    Vertex,     // 顶点缓冲区
    Index,      // 索引缓冲区
    Uniform,    // Uniform 缓冲区
    Storage,    // 存储缓冲区（Vulkan SSBO / D3D12 UAV）
    Staging,    // 暂存缓冲区（用于数据上传/回读）
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

    // 归一化格式（预留，用于纹理）
    R8_UNorm,
    RG8_UNorm,
    RGB8_UNorm,
    RGBA8_UNorm,
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

/// 缓冲区创建描述符
struct RHIBufferDesc
{
    RHIBufferUsage  usage = RHIBufferUsage::Vertex;
    uint64_t        size = 0;               // 缓冲区大小（字节）
    const void*     initialData = nullptr;  // 初始数据（可选）
    std::string     debugName;              // 调试名称（可选）
};

/// 着色器创建描述符
struct RHIShaderDesc
{
    RHIShaderStage          stage = RHIShaderStage::Vertex;
    std::string             filePath;       // 着色器源文件路径（OpenGL: GLSL）
    std::vector<uint8_t>    spirvData;      // SPIR-V 字节码（Vulkan）
    std::string             entryPoint = "main";  // 入口函数名
    std::string             debugName;      // 调试名称（可选）
};

/// 图形管线创建描述符
/// 对应 Vulkan 的 VkGraphicsPipelineCreateInfo（简化版）
struct RHIPipelineDesc
{
    RHIShader*              vertexShader = nullptr;
    RHIShader*              fragmentShader = nullptr;
    RHIVertexInputDesc      vertexInput;
    RHIPrimitiveTopology    topology = RHIPrimitiveTopology::TriangleList;
    RHIRasterizationDesc    rasterization;
    RHIDepthStencilDesc     depthStencil;
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

/// 渲染通道开始信息
/// 对应 Vulkan 的 VkRenderPassBeginInfo（简化版）
struct RHIRenderPassBeginInfo
{
    float       clearColor[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float       clearDepth = 1.0f;
    uint32_t    clearStencil = 0;
    RHIViewport viewport;
};

/// 纹理过滤模式
enum class RHITextureFilter : uint8_t
{
    Nearest,
    Linear,
};

/// 纹理寻址模式
enum class RHITextureAddressMode : uint8_t
{
    Repeat,
    ClampToEdge,
};

/// 2D 纹理创建描述符
struct RHITextureDesc
{
    uint32_t    width = 0;
    uint32_t    height = 0;
    RHIFormat   format = RHIFormat::RGBA8_UNorm;
    const void* initialData = nullptr;
    bool        generateMips = true;
    bool        srgb = true;
    std::string debugName;
};

/// 采样器创建描述符
struct RHISamplerDesc
{
    RHITextureFilter     minFilter = RHITextureFilter::Linear;
    RHITextureFilter     magFilter = RHITextureFilter::Linear;
    RHITextureAddressMode addressU = RHITextureAddressMode::Repeat;
    RHITextureAddressMode addressV = RHITextureAddressMode::Repeat;
    std::string           debugName;
};

} // namespace TE
