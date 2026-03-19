project(tlsf C)

add_library(tlsf STATIC
    ${CMAKE_SOURCE_DIR}/Source/Runtime/Core/Public/Memory/tlsf.c
)

target_include_directories(tlsf PUBLIC
    ${CMAKE_SOURCE_DIR}/Source/Runtime/Core/Public/Memory
)
