project(Asset)

# 创建Asset静态库
add_library(Asset STATIC
        # Public接口
        Public/Texture2D.h

        # Private实现
        Private/Texture2D.cpp
)

# 使用第三方库
target_include_directories(Asset PUBLIC
        ${CMAKE_SOURCE_DIR}/ThirdParty
)