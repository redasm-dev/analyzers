cmake_minimum_required(VERSION 3.20)

project(pdb C)

if(NOT TARGET redasm::redasm)
    find_package(redasm REQUIRED)
endif()

add_library(${PROJECT_NAME} SHARED)

target_sources(${PROJECT_NAME}
    PRIVATE 
        dbi.c
        load.c
        module.c
        pdb.c
        symbols.c
)

target_link_libraries(${PROJECT_NAME} 
    PRIVATE 
        redasm::redasm
)
