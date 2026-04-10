file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

add_library(Core STATIC ${CORE_SOURCES})

target_include_directories(Core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
            ${CMAKE_SOURCE_DIR}/ThirdParty
)

target_link_libraries(Core PRIVATE tlsf)

# 平台宏定义 - PUBLIC 传递给所有依赖 Core 的模块
if(TE_PLATFORM_MACOS)
    target_compile_definitions(Core PUBLIC TE_PLATFORM_MACOS=1)
elseif(TE_PLATFORM_WINDOWS)
    target_compile_definitions(Core PUBLIC TE_PLATFORM_WINDOWS=1)
elseif(TE_PLATFORM_LINUX)
    target_compile_definitions(Core PUBLIC TE_PLATFORM_LINUX=1)
endif()
