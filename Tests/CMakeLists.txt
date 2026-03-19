project(SimpleTest)

add_executable(SimpleTest
	SimpleTest.cpp
)

target_link_libraries(SimpleTest
	PRIVATE
		Core
		glfw
)