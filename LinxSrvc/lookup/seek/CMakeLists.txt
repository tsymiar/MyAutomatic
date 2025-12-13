CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.6)

SET(BUILD_SEEK_TIME_LIB 0)

IF(NOT BUILD_SEEK_TIME_LIB)
    PROJECT(seekTest)
ELSE()
    PROJECT(seekUtil)
ENDIF()

# Set Build type Debug or Release�? default Debug when not set CMAKE_BUILD_TYPE
#SET(CMAKE_BUILD_TYPE Debug CACHE STRING "set build type to debug" FORCE)
#SET(CMAKE_BUILD_TYPE Release CACHE STRING "set build type to release" FORCE)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(..)
SET(SOURCE_FILES
    SeekTime.cpp
    SeekDbMgr.cpp
)

# Locate sqlite3 (headers and library)
find_path(SQLITE3_INCLUDE_DIR sqlite3.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES sqlite3
)
find_library(SQLITE3_LIBRARY NAMES sqlite3
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
)
if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
    message(STATUS "Found SQLite3: ${SQLITE3_LIBRARY}, include: ${SQLITE3_INCLUDE_DIR}")
    include_directories(${SQLITE3_INCLUDE_DIR})
    list(APPEND EXTRA_LIBS ${SQLITE3_LIBRARY})
    add_definitions(-DUSE_SQLITE3)
else()
    message("SQLite3 not found, install libsqlite3-dev (or equivalent) or set SQLITE3_INCLUDE_DIR/SQLITE3_LIBRARY.")
    find_program(APT_GET_EXE apt-get)
    if(APT_GET_EXE)
        execute_process(COMMAND sudo apt-get install -y libsqlite3-dev)
    else()
        message(WARNING "Not found apt-get!")
    endif()
endif()

SET(TARGET_NAME ${PROJECT_NAME})
IF (NOT BUILD_SEEK_TIME_LIB)
    ADD_EXECUTABLE(${PROJECT_NAME} ${SOURCE_FILES} ../test/main.cpp)
ELSE ()
    ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCE_FILES})
    IF (APPLE)
        SET(TARGET_NAME lib${PROJECT_NAME}.dylib)
    ELSE ()
        SET(TARGET_NAME lib${PROJECT_NAME}.so)
    ENDIF ()
ENDIF ()

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${EXTRA_LIBS})

execute_process(
    COMMAND git rev-parse --short=8 HEAD
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE _GIT_COMMIT
    ERROR_QUIET
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-D_GIT_COMMIT=\"${_GIT_COMMIT}\")
file(TIMESTAMP ${CMAKE_BINARY_DIR} BUILT_TIME)
add_definitions(-D_BUILT_TIME="${BUILT_TIME}")

ADD_CUSTOM_COMMAND(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME} ${EXEC_PATH}
)
