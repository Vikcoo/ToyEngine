# Runtime模块
add_subdirectory(Runtime)

# Sandbox示例程序
if(TE_BUILD_SANDBOX)
    add_subdirectory(Sandbox)
endif()

