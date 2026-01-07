project(Input)

add_library(Input STATIC
	Private/InputSystem.cpp

	Public/InputSystem.h
)

target_include_directories(Input
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/Public
	PRIVATE
		${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.4/include
		${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(Input
	PRIVATE
		Core
		Platform
)