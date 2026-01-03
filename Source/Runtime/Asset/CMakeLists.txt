project(Asset)

# 创建Asset静态库
add_library(Asset STATIC
        # Public接口
        Public/Texture2D.h

        # Private实现
        Private/Texture2D.cpp
	Private/Loaders/ModelLoader.cpp
	Private/Loaders/ModelLoader.h
	Private/Mesh.cpp
	Public/Mesh.h
	Private/AssetManager.cpp
	Public/AssetManager.h
	Private/Loaders/TextureLoader.cpp
	Private/Loaders/TextureLoader.h
)

target_include_directories(Asset
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/Public
		${Vulkan_INCLUDE_DIRS}

	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/Private
		${CMAKE_SOURCE_DIR}/ThirdParty

)

target_link_libraries(Asset PRIVATE
	Core
	RHI
)