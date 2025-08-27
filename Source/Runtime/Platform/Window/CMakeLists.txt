add_library(TEWindow STATIC
        Public/TEWindow.h
        Public/TEGLFWWindow.h
        Private/TEWindow.cpp
        Private/TEGLFWWindow.cpp

)

# 配置头文件路径
target_include_directories(TEWindow
        PUBLIC
        # 对外暴露的头文件路径，让其他模块能包含 TELog.h
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>

        PRIVATE
        # TELog 内部私有头文件路径
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(TEWindow PRIVATE
        TELog
        glfw
)