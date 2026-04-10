add_library(Input STATIC
    Private/InputManager.cpp
)

target_include_directories(Input
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_link_libraries(Input
    PUBLIC
        Core
        Platform
)
