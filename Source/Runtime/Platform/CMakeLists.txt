file(GLOB_RECURSE PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

add_library(Platform STATIC ${PLATFORM_SOURCES})

target_include_directories(Platform
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
            ${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.4/include
)

target_link_libraries(Platform
    PUBLIC  Core
    PRIVATE glfw
)
