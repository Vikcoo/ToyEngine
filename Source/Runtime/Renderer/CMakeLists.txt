project(Renderer)


# 创建静态库
add_library(Renderer STATIC
	Public/Renderer.h

	Private/Renderer.cpp
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
	Core
)