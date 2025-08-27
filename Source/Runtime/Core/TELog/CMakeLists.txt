# 定义 TELog 库（静态库，也可改为 SHARED 动态库）
add_library(TELog STATIC
        Public/TELog.h
        Private/TELog.cpp
)

# 配置头文件路径
target_include_directories(TELog
        PUBLIC
        # 对外暴露的头文件路径，让其他模块能包含 TELog.h
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>
        ${CMAKE_SOURCE_DIR}/ThirdParty
        PRIVATE
        # TELog 内部私有头文件路径
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # 第三方

)