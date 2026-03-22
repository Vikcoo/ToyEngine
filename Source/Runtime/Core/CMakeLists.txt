file(GLOB_RECURSE CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

add_library(Core STATIC ${CORE_SOURCES})

target_include_directories(Core
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

# 收窄 ThirdParty 暴露：只包含实际被 Public 头文件依赖的库
# - glm: MathTypes.h 需要
# - spdlog: Log.h 需要（类型出现在接口中）
target_include_directories(Core PUBLIC
    ${CMAKE_SOURCE_DIR}/ThirdParty
)

target_link_libraries(Core PRIVATE tlsf)
