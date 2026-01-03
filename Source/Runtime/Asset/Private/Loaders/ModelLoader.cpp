//
// Created by yukai on 2026/1/3.
//

#include "ModelLoader.h"

#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include "Mesh.h"
#include "Log/Log.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tinyobjloader-release/tiny_obj_loader.h"



// 辅助哈希函数：用于顶点去重（std::unordered_map 需自定义哈希）
template<> struct std::hash<TE::Vertex> {
    size_t operator()(const TE::Vertex& v) const noexcept
    {
        // 简单哈希组合：可使用 boost 或自定义更健壮的哈希
        const size_t hashPos = hash<glm::vec3>()(v.pos);
        const size_t hashTex = hash<glm::vec2>()(v.texCoord);
        const size_t hashNorm = hash<glm::vec3>()(v.nor);
        return hashPos ^ (hashTex << 1) ^ (hashNorm << 2);
    }
};

namespace TE
{
    std::shared_ptr<Mesh> ModelLoader::LoadFromFile(const std::string& filePath)
    {
        auto resultMesh = std::make_shared<Mesh>();
        resultMesh->name = filePath.substr(filePath.find_last_of('/') + 1); // 提取文件名作为网格名

        tinyobj::ObjReader reader;
        tinyobj::ObjReaderConfig readerConfig;

        // 1. 加载OBJ文件并校验结果
        if (!reader.ParseFromFile(filePath, readerConfig)) {
            TE_LOG_ERROR("[ModelLoader] load failed:{},error info：{}",filePath, reader.Error());
            return resultMesh; // 返回无效网格
        }
        if (!reader.Warning().empty()) {
            TE_LOG_WARN("[ModelLoader] load:{} ,warning info:{}",filePath, reader.Warning());
        }

        // 2. 获取tinyobjloader原始数据
        const tinyobj::attrib_t& attrib = reader.GetAttrib();
        const std::vector<tinyobj::shape_t>& shapes = reader.GetShapes();
        if (shapes.empty()) {
            TE_LOG_ERROR("[ModelLoader] {} 中无有效形状数据",filePath);
            return resultMesh;
        }

        // 3. 解析数据：顶点去重 + 组装索引（优化显存占用）
        std::unordered_map<Vertex, uint32_t> vertexMap; // 缓存已存在的顶点，避免重复
        size_t totalIndices = 0;

        for (size_t s = 0; s < shapes.size(); s++) {
            const tinyobj::shape_t& shape = shapes[s];
            const tinyobj::mesh_t& mesh = shape.mesh;
            size_t indexOffset = 0;

            for (size_t f = 0; f < mesh.num_face_vertices.size(); f++) {
                int faceVertexCount = mesh.num_face_vertices[f];
                // 强制三角化（若OBJ是四边形/多边形，此处只取前3个顶点，或扩展三角化逻辑）
                if (faceVertexCount != 3) {
                    TE_LOG_ERROR("[ModelLoader] 警告：{} 中存在非三角形面，已自动三角化！",filePath);
                    faceVertexCount = 3;
                }

                for (size_t v = 0; v < faceVertexCount; v++) {
                    const tinyobj::index_t& idx = mesh.indices[indexOffset + v];
                    Vertex currentVertex;

                    // 提取顶点位置
                    if (idx.vertex_index >= 0) {
                        currentVertex.pos.x = attrib.vertices[3 * idx.vertex_index + 0];
                        currentVertex.pos.y = attrib.vertices[3 * idx.vertex_index + 1];
                        currentVertex.pos.z = attrib.vertices[3 * idx.vertex_index + 2];
                    }

                    // 提取纹理坐标
                    if (idx.texcoord_index >= 0) {
                        currentVertex.texCoord.x = attrib.texcoords[2 * idx.texcoord_index + 0];
                        currentVertex.texCoord.y = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }

                    // 提取法向量
                    if (idx.normal_index >= 0) {
                        currentVertex.nor.x = attrib.normals[3 * idx.normal_index + 0];
                        currentVertex.nor.y = attrib.normals[3 * idx.normal_index + 1];
                        currentVertex.nor.z = attrib.normals[3 * idx.normal_index + 2];
                    }

                    // 顶点去重：若顶点已存在，直接使用原有索引；否则添加新顶点
                    uint32_t vertexIndex;
                    if (vertexMap.find(currentVertex) != vertexMap.end()) {
                        vertexIndex = vertexMap[currentVertex];
                    } else {
                        vertexIndex = static_cast<uint32_t>(resultMesh->m_vertices.size());
                        vertexMap[currentVertex] = vertexIndex;
                        resultMesh->m_vertices.push_back(currentVertex);
                    }

                    // 添加索引
                    resultMesh->m_indices.push_back(vertexIndex);
                    totalIndices++;
                }

                indexOffset += faceVertexCount;
            }
        }

        // 4. 标记网格为有效
        resultMesh->m_isValid = true;
        TE_LOG_DEBUG("[ModelLoader] 加载成功：{}",filePath);
        auto aa = resultMesh->m_vertices.size();
        TE_LOG_DEBUG("[ModelLoader] 顶点数：{}", std::to_string(aa));
        TE_LOG_DEBUG("[ModelLoader] 索引数：{}", std::to_string(resultMesh->m_indices.size()));
        return resultMesh;
    }
} // TE