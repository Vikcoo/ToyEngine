project(tlsf C)

# 使用 OBJECT 库并入 Core 静态库，避免可执行文件仅 link Core 时出现 tlsf_* 未解析符号
add_library(tlsf OBJECT
    tlsf.c
)

target_include_directories(tlsf PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
