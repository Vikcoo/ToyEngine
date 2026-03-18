project(Core)

# 创建Core静态库
add_library(Core STATIC
    # Public接口
    Public/Log/Log.h
    Public/Memory/Memory.h
    Public/Memory/MemoryTag.h
    Public/Memory/tlsf.h
	# Private实现
    Private/Log.cpp
    Private/Memory/Memory.cpp
    Private/Memory/TlsfAllocator.h
    Private/Memory/TlsfAllocator.cpp
    Private/Memory/GlobalNewDelete.cpp
    Public/Memory/tlsf.c
)

# 公共包含目录
target_include_directories(Core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

# 私有包含目录
target_include_directories(Core PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 使用第三方库
target_include_directories(Core PUBLIC
    ${CMAKE_SOURCE_DIR}/ThirdParty
)

message(STATUS "Core module configured")