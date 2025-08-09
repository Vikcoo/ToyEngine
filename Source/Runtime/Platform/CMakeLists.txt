# 定义 TELog 模块为静态库
add_library(TEWindow
    # 公开头文件
    Public/TEWindow.h
    Public/TEGLFWWindow.h
    Public/TEGraphicContext.h
    Public/TEVKGraphicContext.h
    Public/TEVKCommon.h
    # 私有实现文件
    Private/TEWindow.cpp
    Private/TEGLFWWindow.cpp 
    Private/TEGraphicContext.cpp 
    Private/TEVKGraphicContext.cpp
    )

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message("----> find vulkan success: "${Vulkan_INCLUDE_DIRS})
endif()

# 关键：设置头文件路径，Public 目录对外公开
target_include_directories(TEWindow
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>  # 构建时的路径
        $<INSTALL_INTERFACE:Public>  # 安装后路径（可选）
        ${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.4/include  # 添加 spdlog 头文件路径
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private  # 内部实现路径，不对外暴露
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(TEWindow
    PRIVATE
        TELog
        glfw
        ${Vulkan_LIRARY}
)


