# ToyEngine Vulkan 后端

find_package(Vulkan 1.3 REQUIRED)

file(GLOB_RECURSE RHIVULKAN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

add_library(RHIVulkan STATIC ${RHIVULKAN_SOURCES})

target_include_directories(RHIVulkan
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private
            ${CMAKE_SOURCE_DIR}/ThirdParty/glfw-3.4/include
)

target_link_libraries(RHIVulkan
    PRIVATE RHI Vulkan::Vulkan glfw
)

target_compile_definitions(RHIVulkan PRIVATE
    TE_VULKAN_SHADER_ROOT_DIR="${CMAKE_BINARY_DIR}/CompiledShaders/Vulkan"
)

if(TE_VULKAN_VALIDATION)
    target_compile_definitions(RHIVulkan PRIVATE TE_VULKAN_VALIDATION=1)
endif()

message(STATUS "[ToyEngine] Vulkan SDK: ${Vulkan_VERSION}")
message(STATUS "[ToyEngine] Vulkan loader: ${Vulkan_LIBRARY}")
