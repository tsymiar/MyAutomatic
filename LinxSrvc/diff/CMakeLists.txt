CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.16)

set(PROJ_NAME diffs)
project(${PROJ_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CONFIGURE_DEPENDS: re-glob when sources are added or removed.
file(GLOB SRC_FILE_LIST CONFIGURE_DEPENDS "*.c" "*.cc" "*.cpp" "*.cxx")

add_executable(${PROJ_NAME} ${SRC_FILE_LIST})
target_compile_options(${PROJ_NAME} PRIVATE -fPIC -Wall -ggdb)

find_package(Threads REQUIRED)
target_link_libraries(${PROJ_NAME} PRIVATE Threads::Threads ${EXTERNAL_LIBS})

set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)
add_custom_command(
    TARGET ${PROJ_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJ_NAME}> ${EXEC_PATH}
)
