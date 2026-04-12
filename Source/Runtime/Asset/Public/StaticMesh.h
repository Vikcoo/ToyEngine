// ToyEngine Asset Module
// TStaticMesh - 静态网格资产
// 对应 UE5 的 UStaticMesh（简化版）
//
// 设计思想：
// UE5 中 UStaticMesh 持有 LOD 数据（FStaticMeshLODResources），
// 每个 LOD 包含多个 Section（子网格），每个 Section 对应一个材质。
//
// ToyEngine 简化版：
// - 暂不实现 LOD，只保留一级
// - TStaticMesh 持有 vector<FMeshSection>，每个 Section 包含独立的顶点和索引数据
// - 顶点结构统一为 FStaticMeshVertex（Position + Normal + TexCoord + Color）
// - 资产可以被多个 TMeshComponent 共享引用（通过 shared_ptr）

#pragma once

#include "Math/MathTypes.h"
#include <vector>
#include <string>
#include <cstdint>

namespace TE {

/// 静态网格顶点（对应 UE5 FStaticMeshVertex）
///
/// 包含渲染所需的全部逐顶点属性：
/// - Position: 模型空间坐标
/// - Normal:   法线方向（用于光照计算）
/// - TexCoord: 纹理坐标（UV映射）
/// - Color:    顶点色（用于无纹理时的着色）
struct FStaticMeshVertex
{
    Vector3 Position;   // 位置     (12 bytes)
    Vector3 Normal;     // 法线     (12 bytes)
    Vector2 TexCoord;   // 纹理坐标 (8 bytes)
    Vector3 Color;      // 顶点色   (12 bytes)
    // Total: 44 bytes per vertex
};

/// 子网格 / 段（对应 UE5 FStaticMeshSection）
///
/// 每个 Section 对应模型中的一个子网格，通常对应一个材质。
/// 例如一个角色模型可能有：身体(Section 0) + 武器(Section 1) + 头发(Section 2)
struct FMeshSection
{
    std::vector<FStaticMeshVertex>  Vertices;           // 该 Section 的顶点数据
    std::vector<uint32_t>           Indices;            // 该 Section 的索引数据
    uint32_t                        MaterialIndex = 0;  // 材质索引
};

/// 静态网格材质槽（简化版）
/// 当前仅保留 BaseColor 纹理路径，后续可扩展到法线/金属度/粗糙度等贴图。
struct FStaticMeshMaterial
{
    std::string BaseColorTexturePath;

    [[nodiscard]] bool HasBaseColorTexture() const { return !BaseColorTexturePath.empty(); }
};

/// 静态网格资产（对应 UE5 UStaticMesh）
///
/// 核心职责：
/// - 持有 CPU 侧的网格数据（顶点 + 索引 + Section 划分）
/// - 作为资产被多个 TMeshComponent 共享引用
/// - 由 FAssetImporter 加载创建
///
/// 生命周期：
/// FAssetImporter::ImportStaticMesh(path)
///   → 创建 TStaticMesh，填充 Section 数据
///   → 返回 shared_ptr<TStaticMesh>
///   → TMeshComponent::SetStaticMesh() 引用
///   → FStaticMeshSceneProxy 在构造时只保存资产引用
///   → Renderer 侧资源准备阶段再将资产解析为 GPU 资源
class StaticMesh
{
public:
    StaticMesh() = default;
    ~StaticMesh() = default;

    // ==================== 查询接口 ====================

    /// 获取所有子网格段（SceneProxy 遍历用）
    [[nodiscard]] const std::vector<FMeshSection>& GetSections() const { return m_Sections; }

    /// 是否包含有效的网格数据
    [[nodiscard]] bool IsValid() const;

    /// 获取资产名称（通常为文件名）
    [[nodiscard]] const std::string& GetName() const { return m_Name; }

    /// 获取所有 Section 的总顶点数
    [[nodiscard]] uint32_t GetTotalVertexCount() const;

    /// 获取所有 Section 的总索引数
    [[nodiscard]] uint32_t GetTotalIndexCount() const;

    /// 获取 Section 数量
    [[nodiscard]] uint32_t GetSectionCount() const { return static_cast<uint32_t>(m_Sections.size()); }

    /// 获取材质槽
    [[nodiscard]] const std::vector<FStaticMeshMaterial>& GetMaterials() const { return m_Materials; }
    [[nodiscard]] const FStaticMeshMaterial* GetMaterial(uint32_t materialIndex) const;

    // ==================== 构建接口（供 FAssetImporter 使用） ====================

    /// 设置资产名称
    void SetName(const std::string& name) { m_Name = name; }

    /// 添加一个子网格段
    void AddSection(FMeshSection section);

    /// 设置材质槽（由导入器填充）
    void SetMaterials(std::vector<FStaticMeshMaterial> materials) { m_Materials = std::move(materials); }

private:
    std::string               m_Name;       // 资产名称（通常为文件名）
    std::vector<FMeshSection> m_Sections;   // 所有子网格段
    std::vector<FStaticMeshMaterial> m_Materials; // 材质槽
};

} // namespace TE
