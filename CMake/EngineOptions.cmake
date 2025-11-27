# 引擎构建选项

# 图形API选项
option(TE_RHI_VULKAN "Enable Vulkan RHI backend" ON)
option(TE_RHI_D3D12 "Enable DirectX 12 RHI backend" OFF)
option(TE_RHI_OPENGL "Enable OpenGL RHI backend" OFF)

# 检查至少有一个RHI后端被启用
if(NOT (TE_RHI_VULKAN OR TE_RHI_D3D12 OR TE_RHI_OPENGL))
    message(FATAL_ERROR "At least one RHI backend must be enabled!")
endif()

# 设置RHI宏定义
if(TE_RHI_VULKAN)
    message(STATUS "RHI Backend: Vulkan enabled")
    add_definitions(-DTE_RHI_VULKAN)
endif()

if(TE_RHI_D3D12)
    message(STATUS "RHI Backend: DirectX 12 enabled")
    add_definitions(-DTE_RHI_D3D12)
endif()

if(TE_RHI_OPENGL)
    message(STATUS "RHI Backend: OpenGL enabled")
    add_definitions(-DTE_RHI_OPENGL)
endif()

# 其他构建选项
option(TE_BUILD_SANDBOX "Build sandbox application" ON)
option(TE_BUILD_TESTS "Build unit tests" OFF)
option(TE_BUILD_DOCS "Build documentation" OFF)
option(TE_ENABLE_PROFILING "Enable profiling" OFF)
option(TE_ENABLE_LOGGING "Enable logging" ON)

if(TE_ENABLE_PROFILING)
    add_definitions(-DTE_ENABLE_PROFILING)
    message(STATUS "Profiling: Enabled")
endif()

if(TE_ENABLE_LOGGING)
    add_definitions(-DTE_ENABLE_LOGGING)
    message(STATUS "Logging: Enabled")
endif()

