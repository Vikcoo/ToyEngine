# 定义 TELog 模块为静态库
add_library(TELog 
    # 公开头文件
    Public/TELog.h

    # 私有实现文件
    Private/TELog.cpp
)

# 关键：设置头文件路径，Public 目录对外公开
target_include_directories(TELog
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>  # 构建时的路径
        $<INSTALL_INTERFACE:Public>  # 安装后路径（可选）
        ${CMAKE_SOURCE_DIR}/ThirdParty  # 添加 spdlog 头文件路径
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private  # 内部实现路径，不对外暴露

)

    