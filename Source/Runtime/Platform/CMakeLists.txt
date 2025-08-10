# ���� TELog ģ��Ϊ��̬��
add_library(TEWindow
    # ����ͷ�ļ�
    Public/TEWindow.h
    Public/TEGLFWWindow.h
    Public/TEGraphicContext.h
    Public/TEVKGraphicContext.h
    Public/TEVKCommon.h
    # ˽��ʵ���ļ�
    Private/TEWindow.cpp
    Private/TEGLFWWindow.cpp 
    Private/TEGraphicContext.cpp 
    Private/TEVKGraphicContext.cpp
    )

find_package(Vulkan REQUIRED)
if(Vulkan_FOUND)
    message("----> find vulkan success: " ${Vulkan_INCLUDE_DIRS})
endif()

# �ؼ�������ͷ�ļ�·����Public Ŀ¼���⹫��
target_include_directories(TEWindow
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>  # ����ʱ��·��
        $<INSTALL_INTERFACE:Public>  # ��װ��·������ѡ��
        ${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.4/include  # ��� spdlog ͷ�ļ�·��
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private  # �ڲ�ʵ��·���������Ⱪ¶
        ${Vulkan_INCLUDE_DIRS}
)

target_link_libraries(TEWindow
    PRIVATE
        TELog
        glfw
        ${Vulkan_LIBRARY}
)


