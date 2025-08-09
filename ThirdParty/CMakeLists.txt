# 添加glfw之前关闭一些选项
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_TESTS OFF)
option(GLFW_BUILD_EXAMPLE OFF)
option(GLFW_INSTALL OFF)
# 添加glfw子目录
add_subdirectory(glfw-3.4)