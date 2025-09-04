/*
文件用途: 资源路径与文件工具
- 负责: 统一资源目录宏定义、常用文件名/大小/时间格式化工具、二进制读取
- 说明: 与 Vulkan 无强耦合，但在加载着色器(.spv)等场景中常用
*/
#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef TE_DEFINE_RES_ROOT_DIR
#define TE_RES_ROOT_DIR TE_DEFINE_RES_ROOT_DIR
#else
#define TE_RES_ROOT_DIR "Content/"
#endif

#define TE_RES_CONFIG_DIR TE_RES_ROOT_DIR"Config/"
#define TE_RES_SHADER_DIR TE_RES_ROOT_DIR"Shaders/"
#define TE_RES_FONT_DIR TE_RES_ROOT_DIR"Font/"
#define TE_RES_MODEL_DIR TE_RES_ROOT_DIR"Model/"
#define TE_RES_MATERIAL_DIR TE_RES_ROOT_DIR"Material/"
#define TE_RES_TEXTURE_DIR TE_RES_ROOT_DIR"Texture/"
#define TE_RES_SCENE_DIR TE_RES_ROOT_DIR"Scene/"

namespace TE {
    // 提取文件名（不含路径）
    static std::string GetFileName(const std::string &filePath) {
        if (filePath.empty()) {
            return filePath;
        }
        // 修正：std::filesystem 中间不应有空格
        std::filesystem::path path(filePath);
        return path.filename().string();
    }

    // 将字节数格式化为带单位字符串（B/KB/MB/GB）
    static void FormatFileSize(std::uintmax_t filesize, float* outsize, std::string& outUnit) {
        float size = static_cast<float>(filesize);

        if (size < 1024) {
            outUnit = "B";  // 字节
        } else if (size < 1024 * 1024) {
            size /= 1024;   // 转换为KB
            outUnit = "KB";
        } else if (size < 1024 * 1024 * 1024) {
            size /= (1024 * 1024);  // 转换为MB
            outUnit = "MB";
        } else {
            size /= (1024 * 1024 * 1024);  // 转换为GB
            outUnit = "GB";
        }

        *outsize = size;  // 输出转换后的大小
    }

    // 将文件时间转换为格式化的字符串（年/月/日 时:分）
    static std::string FormatSystemTime(std::filesystem::file_time_type fileTime) {
        // 转换为time_t类型
        std::time_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        // 转换为本地时间
        std::tm* tm = std::localtime(&time);
        if (tm == nullptr) {
            return "Invalid time";
        }

        // 格式化输出
        std::stringstream ss;
        ss << std::put_time(tm, "%Y/%m/%d %H:%M");

        return  ss.str();
    }

    // 从文件读取二进制数据到字符向量（用于读取 .spv 着色器等）
    static std::vector<char> ReadCharArrayFromFile(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("Could not open the file: " + filePath);
        }

        auto fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
}