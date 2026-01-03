//
// Created by yukai on 2026/1/3.
//
#pragma once

#include <string>
#include <vector>
#include <memory>

namespace TE {
    class Mesh;
    class ModelLoader {
    public:
        // 从OBJ文件加载模型
        static std::shared_ptr<Mesh> LoadFromFile(const std::string& filePath);
    };

} // namespace TE