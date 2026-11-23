cmake_minimum_required(VERSION 3.20)
project(redasm-analyzers C)

if(NOT TARGET redasm::redasm)
    find_package(redasm REQUIRED)
endif()

add_subdirectory(compilers)
add_subdirectory(pdb)

redasm_install_plugins(analyzers)
