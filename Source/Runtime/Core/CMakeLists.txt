# ���� TELog ģ��Ϊ��̬��
add_library(TELog 
    # ����ͷ�ļ�
    Public/TELog.h

    # ˽��ʵ���ļ�
    Private/TELog.cpp
)

# �ؼ�������ͷ�ļ�·����Public Ŀ¼���⹫��
target_include_directories(TELog
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>  # ����ʱ��·��
        $<INSTALL_INTERFACE:Public>  # ��װ��·������ѡ��
        ${CMAKE_SOURCE_DIR}/ThirdParty  # ��� spdlog ͷ�ļ�·��
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private  # �ڲ�ʵ��·���������Ⱪ¶

)

    