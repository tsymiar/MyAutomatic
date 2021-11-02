cmake_minimum_required (VERSION 3.8)

project ("HttpAgent")

set(TARGET HttpAgent)

file(GLOB SRC_FILE_LIST "*.cpp")

add_executable (${TARGET} ${SRC_FILE_LIST})

include_directories (
    /usr/local/include
    /usr/include
)

find_library(EVENT_LIB event ${APPOINT_EVENT}/lib /usr/lib /usr/local/lib NOTFOUND)

add_definitions(-std=c++11)

target_link_libraries (${TARGET}
    -fPIC
    -Wall
    -ggdb
    -pthread
    ${EVENT_LIB}
)

if (DEFINED ENV{WITH_LEVELDB})
    include_directories (${WITH_LEVELDB}/include)
    find_library(LEVELDB_LIB leveldb ${WITH_LEVELDB}/lib /usr/local/lib NOTFOUND)
    if (NOT LEVELDB_LIB STREQUAL "LEVELDB_LIB-NOTFOUND")
        target_link_libraries (${TARGET} ${LEVELDB_LIB})
    endif()
endif()
