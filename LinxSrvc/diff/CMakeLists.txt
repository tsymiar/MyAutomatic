CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.16)

set(CXX_STANDARD 17)
set(PROJ_NAME analyzer)
project(${PROJ_NAME} C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fPIC -Wall -ggdb")

file(GLOB SRC_FILE_LIST "*[^.h]*.c*[^.txt]*" "*.hpp")

add_executable(${PROJ_NAME} ${SRC_FILE_LIST})
target_link_libraries(${PROJ_NAME} -pthread ${EXTERNAL_LIBS})

set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)
add_custom_command(
    TARGET ${PROJ_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJ_NAME}> ${EXEC_PATH}
)
