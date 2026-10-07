CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.16)

# lookup/test - end-to-end driver for lookup.
# One main.cpp covers both engine generations, switched by the TIMESEEK macro:
#   V1 seek/  no macro : built as seek_test by seek/CMakeLists.txt (BUILD_SEEK_TIME_LIB=0)
#   V2 time/  with it  : built here as lookup_test_v2 (this branch had no target at all)
PROJECT(lookup_test CXX)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

INCLUDE_DIRECTORIES(..)

# Passed down by lookup/CMakeLists.txt; fall back to ../../gen when configured standalone
IF(NOT EXEC_PATH)
    SET(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../gen)
ENDIF()

# sql/Sqlite.hxx only reaches the real library under USE_SQLITE3, and degrades to a
# no-op stub without it, so probe explicitly here
find_path(SQLITE3_INCLUDE_DIR sqlite3.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES sqlite3
)
find_library(SQLITE3_LIBRARY NAMES sqlite3
    PATHS /usr/lib /usr/local/lib /usr/lib/x86_64-linux-gnu /usr/lib/aarch64-linux-gnu
)
if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
    message(STATUS "lookup/test: Found SQLite3: ${SQLITE3_LIBRARY}")
    include_directories(${SQLITE3_INCLUDE_DIR})
    list(APPEND EXTRA_LIBS ${SQLITE3_LIBRARY})
    add_definitions(-DUSE_SQLITE3)
else()
    message(WARNING "lookup/test: SQLite3 unavailable, building with the no-op stub (STUB) - open() always false / execute() always -1 / query() always nullptr")
endif()

ADD_EXECUTABLE(lookup_test_v2 main.cpp ../time/TimeSeek.cpp ../time/TimeDbMgr.cpp)
TARGET_COMPILE_DEFINITIONS(lookup_test_v2 PRIVATE TIMESEEK)
IF(NOT WIN32)
    TARGET_LINK_LIBRARIES(lookup_test_v2 ${EXTRA_LIBS} pthread)
ELSE()
    TARGET_LINK_LIBRARIES(lookup_test_v2 ${EXTRA_LIBS})
ENDIF()

ADD_CUSTOM_COMMAND(
    TARGET lookup_test_v2
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/lookup_test_v2 ${EXEC_PATH}
)
