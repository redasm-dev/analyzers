cmake_minimum_required(VERSION 3.20)

project(compilers C)

if(NOT TARGET redasm::redasm)
    find_package(redasm REQUIRED)
endif()

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE 
        rtti/msvc/msvc.c
        rtti/msvc/types.c
        vb/components.c
        vb/decompiler.c
        vb/format.c
        plugin.c
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        redasm::redasm
)
