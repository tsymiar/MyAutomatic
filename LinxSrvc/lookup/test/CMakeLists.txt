CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.16)

# lookup/test - end-to-end driver for lookup.
# One main.cpp covers both engine generations, switched by the TIMESEEK macro:
#   V1 seek/  no macro : built as seek_test by seek/CMakeLists.txt (BUILD_SEEK_TIME_LIB=0)
#   V2 time/  with it  : built here as lookup_test_v2 (this branch had no target at all)
# The engine itself is not compiled here: lookup_test_v2 links whichever of
# libseekUtil.so / libtimeUtil.so is available, and is skipped when neither is.
PROJECT(lookup_test CXX)

SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

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

# Engine shared library to bind against, both are produced into ${EXEC_PATH}:
#   timeUtil  V2 engine (lookup/time), needs the TIMESEEK macro
#   seekUtil  V1 engine (lookup/seek), no macro
# Whichever one exists is linked: LOOKUP_ENGINE_LIB only sets the probe order,
# the other engine is picked up when the preferred one is missing.
SET(LOOKUP_ENGINE_LIB "timeUtil" CACHE STRING "Preferred engine library: timeUtil (V2) or seekUtil (V1)")
SET_PROPERTY(CACHE LOOKUP_ENGINE_LIB PROPERTY STRINGS timeUtil seekUtil)

IF(LOOKUP_ENGINE_LIB STREQUAL "seekUtil")
    SET(_ENGINE_ORDER seekUtil timeUtil)
ELSE()
    SET(_ENGINE_ORDER timeUtil seekUtil)
ENDIF()

# lookup_test_v2 carries no engine source: it only links the shared library.
# For each engine the CMake target wins (so the library is rebuilt first), then
# the copy already installed in ${EXEC_PATH}.
SET(_ENGINE_NAME)
SET(_ENGINE_LINK)
FOREACH(_engine ${_ENGINE_ORDER})
    FIND_LIBRARY(LOOKUP_${_engine}_LIBRARY
        NAMES ${_engine}
        PATHS ${EXEC_PATH} ${CMAKE_BINARY_DIR}/seek ${CMAKE_BINARY_DIR}/time
        NO_DEFAULT_PATH
    )
    IF(TARGET ${_engine})
        SET(_ENGINE_NAME ${_engine})
        SET(_ENGINE_LINK ${_engine})
        MESSAGE(STATUS "lookup/test: linking engine target ${_engine}")
        BREAK()
    ELSEIF(LOOKUP_${_engine}_LIBRARY)
        SET(_ENGINE_NAME ${_engine})
        SET(_ENGINE_LINK ${LOOKUP_${_engine}_LIBRARY})
        MESSAGE(STATUS "lookup/test: linking engine library ${_ENGINE_LINK}")
        BREAK()
    ENDIF()
ENDFOREACH()

IF(_ENGINE_NAME STREQUAL "seekUtil")
    SET(_ENGINE_DEFS)
ELSE()
    SET(_ENGINE_DEFS TIMESEEK)
ENDIF()

IF(NOT _ENGINE_LINK)
    # Skip this target only: no target is added, so make continues with the rest
    # of the tree, and MESSAGE(WARNING) does not abort the configure step.
    MESSAGE(WARNING "lookup/test: neither seekUtil nor timeUtil found (searched ${EXEC_PATH}, ${CMAKE_BINARY_DIR}/seek, ${CMAKE_BINARY_DIR}/time) - skipping lookup_test_v2, build one of them first")
ELSE()
    ADD_EXECUTABLE(lookup_test_v2 main.cpp)
    TARGET_COMPILE_DEFINITIONS(lookup_test_v2 PRIVATE ${_ENGINE_DEFS})
    IF(NOT WIN32)
        TARGET_LINK_LIBRARIES(lookup_test_v2 ${EXTRA_LIBS} ${_ENGINE_LINK} pthread)
    ELSE()
        TARGET_LINK_LIBRARIES(lookup_test_v2 ${EXTRA_LIBS} ${_ENGINE_LINK})
    ENDIF()

    # driver and engine .so both land in ${EXEC_PATH}; bake it into RPATH so the
    # loader finds the library without LD_LIBRARY_PATH
    SET_TARGET_PROPERTIES(lookup_test_v2 PROPERTIES
        BUILD_RPATH "${EXEC_PATH}"
        INSTALL_RPATH "${EXEC_PATH}"
    )
    # No copy of the engine .so here: seek/ and time/ already install it into
    # ${EXEC_PATH} themselves, and copying a file onto itself truncates it.
    ADD_CUSTOM_COMMAND(
        TARGET lookup_test_v2
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/lookup_test_v2 ${EXEC_PATH}
    )
ENDIF()
