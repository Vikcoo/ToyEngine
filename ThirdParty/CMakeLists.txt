# 添加glfw之前关闭一些选项
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLE OFF)
option(GLFW_INSTALL OFF)
# 添加glfw子目录
add_subdirectory(glfw-3.4)

# TLSF allocator
add_subdirectory(tlsf)

# glad - OpenGL Loader
add_subdirectory(glad)

# ==============================================
# Assimp - Open Asset Import Library
# ==============================================
# 作为子项目集成，关闭不需要的功能以加速编译
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)          # 静态链接，避免动态库部署问题
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)         # 不构建测试
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)  # 不构建辅助工具
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)       # 不构建示例
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)          # 不构建文档
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)             # 作为子目录使用，不需要 install 规则
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)  # 第三方库的警告不应阻断编译
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)            # 我们只需要导入功能，关闭导出以减小体积
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE) # 简化库名，不注入 debug 后缀
set(ASSIMP_IGNORE_GIT_HASH ON CACHE BOOL "" FORCE)      # 子目录模式下不调用 git

add_subdirectory(assimp)
