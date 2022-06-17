cmake_minimum_required (VERSION 3.0)

set(TARGET test)

project ("${TARGET}")
set(CXX_STANDARD 11)

add_definitions(-std=c++11)
add_subdirectory(googletest)

include_directories (
    ../include
    googletest/googletest/include/gtest
)
link_directories (${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

file (GLOB SRC_FILE_LIST "*.c*")
add_executable (${TARGET} ${SRC_FILE_LIST})

target_link_libraries (${TARGET}
    gmock_main
    gmock
    gtest_main
    gtest
    -Wall
    -ggdb
    -lc++
)
