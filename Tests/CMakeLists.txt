project(SimpleTest)

find_package(Vulkan REQUIRED)

add_executable(SimpleTest
	SimpleTest.cpp
)

#target_include_directories(RHI PUBLIC
#	${CMAKE_CURRENT_SOURCE_DIR}/Public
#	${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan  # 暴露 Vulkan 头文件
#	${Vulkan_INCLUDE_DIRS}  # 暴露 Vulkan SDK（VulkanContext.h 依赖 vulkan_raii.hpp）
#)

target_link_libraries(SimpleTest
	PRIVATE
		Core
		Vulkan::Vulkan
		glfw
)