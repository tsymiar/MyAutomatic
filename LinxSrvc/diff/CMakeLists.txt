cmake_minimum_required(VERSION 2.8...3.16)

set(PROJ_NAME diffs)
project(${PROJ_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -Wall -ggdb")

file(GLOB SRC_FILE_LIST "*[^.h]*.c*[^.txt]*" "*.hpp")

add_executable(${PROJ_NAME} ${SRC_FILE_LIST})
target_link_libraries(${PROJ_NAME} PRIVATE -pthread ${EXTERNAL_LIBS})

set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)
add_custom_command(
    TARGET ${PROJ_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJ_NAME}> ${EXEC_PATH}
)
