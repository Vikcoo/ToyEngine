// ToyEngine Asset Module
// FAssetImporter - 模型加载器
// 对应 UE5 的 FAssetRegistry / AssetManager（极简版）
//
// 设计思想：
// - Assimp 的唯一调用方，对外只暴露引擎自有类型
// - Assimp 头文件不会泄漏到其他模块（仅在 .cpp 中 #include）
// - 接口简洁：ImportStaticMesh(path) → shared_ptr<TStaticMesh>
// - 处理 Assimp 场景树递归遍历、属性缺失 fallback 等细节

#pragma once

#include <memory>
#include <string>

namespace TE {

class StaticMesh;

/// 模型加载器
///
/// UE5 映射：
/// - FAssetRegistry: 管理资产的发现、加载、引用
/// - FbxImporter / StaticMesh 构建工具链
///
/// ToyEngine 简化版：
/// - 静态工具类，不需要实例化
/// - 唯一公共接口：ImportStaticMesh()
/// - 内部封装 Assimp 调用，对外完全透明
class FAssetImporter
{
public:
    /// 从文件导入静态网格
    ///
    /// @param filePath 模型文件路径（支持 OBJ、FBX、glTF 等 Assimp 支持的格式）
    /// @return 成功返回 TStaticMesh 资产，失败返回 nullptr
    ///
    /// 处理流程：
    /// 1. Assimp::Importer.ReadFile(filePath, postProcessFlags)
    /// 2. 递归遍历 aiNode，收集所有 aiMesh
    /// 3. 每个 aiMesh → FMeshSection（提取 Position/Normal/UV/Color/Indices）
    /// 4. 对缺失属性做 fallback（无 Normal → (0,0,1)，无 UV → (0,0)，无 Color → 白色）
    /// 5. 组装 TStaticMesh 返回
    [[nodiscard]] static std::shared_ptr<StaticMesh> ImportStaticMesh(const std::string& filePath);
};

} // namespace TE
