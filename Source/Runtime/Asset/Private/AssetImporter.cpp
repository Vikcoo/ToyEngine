// ToyEngine Asset Module
// FAssetImporter 实现 - Assimp 封装层
//
// Assimp 头文件仅在此处引用，不泄漏到 Public 接口
// 这是 UE5 中 "模块内部实现细节不暴露" 原则的体现

#include "AssetImporter.h"
#include "StaticMesh.h"
#include "Log/Log.h"

// Assimp 头文件（仅在 Private 中引用）
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <filesystem>

namespace TE {

/// 从 aiMesh 提取数据，转换为引擎自有的 FMeshSection
static FMeshSection ProcessAiMesh(const aiMesh* mesh)
{
    FMeshSection section;
    section.MaterialIndex = mesh->mMaterialIndex;

    // 预分配内存
    section.Vertices.reserve(mesh->mNumVertices);
    section.Indices.reserve(mesh->mNumFaces * 3);

    // ========== 提取顶点数据 ==========
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
    {
        FStaticMeshVertex vertex;

        // Position（必定存在）
        vertex.Position = Vector3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        // Normal（可能缺失，fallback 为 (0, 0, 1)）
        if (mesh->HasNormals())
        {
            vertex.Normal = Vector3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }
        else
        {
            vertex.Normal = Vector3(0.0f, 0.0f, 1.0f);
        }

        // TexCoord（取第一组 UV，可能缺失，fallback 为 (0, 0)）
        // Assimp 支持最多 8 组 UV，我们只使用第一组
        if (mesh->HasTextureCoords(0))
        {
            vertex.TexCoord = Vector2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }
        else
        {
            vertex.TexCoord = Vector2(0.0f, 0.0f);
        }

        // Color（取第一组顶点色，可能缺失，fallback 为白色）
        if (mesh->HasVertexColors(0))
        {
            vertex.Color = Vector3(
                mesh->mColors[0][i].r,
                mesh->mColors[0][i].g,
                mesh->mColors[0][i].b
            );
        }
        else
        {
            vertex.Color = Vector3(1.0f, 1.0f, 1.0f);
        }

        section.Vertices.push_back(vertex);
    }

    // ========== 提取索引数据 ==========
    // 由于使用了 aiProcess_Triangulate，每个 face 都是三角形
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j)
        {
            section.Indices.push_back(face.mIndices[j]);
        }
    }

    return section;
}

/// 递归遍历 Assimp 场景节点树，收集所有网格
/// 这对应 UE5 中 FBX Importer 遍历 FbxNode 树的过程
static void ProcessAiNode(const aiNode* node, const aiScene* scene,
                           std::shared_ptr<StaticMesh>& outMesh)
{
    // 处理当前节点的所有网格
    for (unsigned int i = 0; i < node->mNumMeshes; ++i)
    {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        FMeshSection section = ProcessAiMesh(mesh);
        outMesh->AddSection(std::move(section));
    }

    // 递归处理子节点
    for (unsigned int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessAiNode(node->mChildren[i], scene, outMesh);
    }
}

/// 构建静态网格材质槽（当前仅导入 BaseColor / Diffuse 贴图路径）
static std::vector<FStaticMeshMaterial> BuildMaterials(const aiScene* scene, const std::filesystem::path& baseDir)
{
    std::vector<FStaticMeshMaterial> materials;
    if (!scene || scene->mNumMaterials == 0)
    {
        return materials;
    }

    materials.resize(scene->mNumMaterials);
    for (unsigned int materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
    {
        const aiMaterial* aiMat = scene->mMaterials[materialIndex];
        if (!aiMat)
        {
            continue;
        }

        aiString texPath;
        aiReturn result = aiMat->GetTexture(aiTextureType_BASE_COLOR, 0, &texPath);
        if (result != aiReturn_SUCCESS)
        {
            result = aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
        }

        if (result != aiReturn_SUCCESS || texPath.length == 0)
        {
            continue;
        }

        std::filesystem::path texturePath = std::filesystem::path(texPath.C_Str());
        if (texturePath.is_relative())
        {
            texturePath = baseDir / texturePath;
        }
        texturePath = texturePath.lexically_normal();
        materials[materialIndex].BaseColorTexturePath = texturePath.string();
    }

    return materials;
}

std::shared_ptr<StaticMesh> FAssetImporter::ImportStaticMesh(const std::string& filePath)
{
    TE_LOG_INFO("[Asset] Importing static mesh: {}", filePath);

    // 创建 Assimp Importer（RAII，析构时自动释放 aiScene）
    Assimp::Importer importer;

    // 后处理标志：
    // - Triangulate:           将所有面转为三角形（必须）
    // - GenSmoothNormals:      对缺失法线的网格生成平滑法线
    // - JoinIdenticalVertices: 合并相同顶点，减少顶点数量
    // - CalcTangentSpace:      计算切线空间（为将来法线贴图预留）
    //
    // 注意：不使用 aiProcess_FlipUVs。引擎统一约定纹理原点为左上角（与图片文件存储顺序一致），
    // 和 Vulkan/D3D12 原生行为对齐。OpenGL 后端在纹理上传时自行翻转像素行序来适配。
    const unsigned int postProcessFlags =
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices |
        aiProcess_CalcTangentSpace;

    const aiScene* scene = importer.ReadFile(filePath, postProcessFlags);

    // 检查加载结果
    if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
    {
        TE_LOG_ERROR("[Asset] Failed to import '{}': {}", filePath, importer.GetErrorString());
        return nullptr;
    }

    // 创建 TStaticMesh 资产
    auto staticMesh = std::make_shared<StaticMesh>();

    // 从文件路径提取名称
    std::filesystem::path path(filePath);
    staticMesh->SetName(path.stem().string());
    staticMesh->SetMaterials(BuildMaterials(scene, path.parent_path()));

    // 递归遍历场景树，收集所有网格
    ProcessAiNode(scene->mRootNode, scene, staticMesh);

    if (!staticMesh->IsValid())
    {
        TE_LOG_ERROR("[Asset] Imported mesh '{}' has no valid data", filePath);
        return nullptr;
    }

    TE_LOG_INFO("[Asset] Successfully imported '{}': {} sections, {} vertices, {} indices",
                staticMesh->GetName(),
                staticMesh->GetSectionCount(),
                staticMesh->GetTotalVertexCount(),
                staticMesh->GetTotalIndexCount());

    return staticMesh;
}

} // namespace TE
