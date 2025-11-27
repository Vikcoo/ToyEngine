project(Platform)

# 创建Platform静态库
add_library(Platform STATIC
    # Public接口
    Public/Window.h
    
    # Private实现
    Private/PlatformFactory.cpp
    Private/GLFW/GLFWWindow.h
    Private/GLFW/GLFWWindow.cpp
)

# 公共包含目录（Public接口）
target_include_directories(Platform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

# 私有包含目录（Private实现）
target_include_directories(Platform PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 链接依赖
target_link_libraries(Platform 
    PUBLIC
        Core  # Platform需要使用日志
    PRIVATE
        glfw
)

# GLFW包含目录
target_include_directories(Platform PRIVATE
    ${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.4/include
)

message(STATUS "Platform module configured")

