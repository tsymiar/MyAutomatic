cmake_minimum_required(VERSION 3.0)

project(kaics)

set(CMAKE_CXX_STANDARD 14)
set(TARGET kaics.exe)

file(GLOB SRC_FILE_LIST "Kai*")

add_executable (${TARGET} ${SRC_FILE_LIST})

target_link_libraries (${TARGET}
        -fPIC
        -Wall
        -ggdb)
