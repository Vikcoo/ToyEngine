project(Renderer)


# 创建静态库
add_library(Renderer STATIC
	Public/VulkanRenderer.h
	Private/VulkanRenderer.cpp
)

# 包含目录
target_include_directories(Renderer
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/Public
	PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 链接依赖
target_link_libraries(Renderer
	PUBLIC
	RHI
	Platform
	Core
)

# 定义项目根目录路径（用于运行时查找资源）
# 将路径中的反斜杠转换为正斜杠，并转义引号
string(REPLACE "\\" "/" PROJECT_ROOT_NORMALIZED "${CMAKE_SOURCE_DIR}")
target_compile_definitions(Renderer PRIVATE
	TE_PROJECT_ROOT_DIR="${PROJECT_ROOT_NORMALIZED}"
)
