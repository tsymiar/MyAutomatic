cmake_minimum_required(VERSION 2.8...3.16)

set(TARGET webevs_serve)

project ("${TARGET}")
add_definitions(-std=c++11)
set(CXX_STANDARD 11)

include_directories (
    /usr/local/include
    /usr/include
)

find_library(EVENT_LIB event ${APPOINT_EVENT}/lib /usr/lib /usr/local/lib /usr/lib/arm-linux-gnueabihf NOTFOUND)

# optional: standalone websocket server (libwebsockets)
find_library(WEBSOCKETS_LIB websockets ${APPOINT_EVENT}/lib /usr/lib /usr/local/lib /usr/lib/aarch64-linux-gnu NOTFOUND)
if (NOT ${WEBSOCKETS_LIB} MATCHES "NOTFOUND")
    add_definitions(-DWITH_WEBSOCKETS)
else()
    message(--\twebsocket\tlibrary\tnot\tfound,\tWS\tserver\tdisabled.)
endif()

if (NOT ${EVENT_LIB} MATCHES "NOTFOUND")
    file(GLOB SRC_FILE_LIST "*[^.h]*.c*[^.txt]*")
    add_executable (${TARGET} ${SRC_FILE_LIST})
    target_link_libraries (${TARGET}
        -fPIC
        -Wall
        -ggdb
        -pthread
        ${EVENT_LIB}
    )
    if (NOT ${WEBSOCKETS_LIB} MATCHES "NOTFOUND")
        target_link_libraries (${TARGET} ${WEBSOCKETS_LIB})
    endif()
else()
    message(--\tseek\tlibrary\tvalues\t'${EVENT_LIB}',\tprocess\tmay\tnot\texecute\tnormaly.)
endif()

if (DEFINED ENV{WITH_LEVELDB})
    include_directories (${WITH_LEVELDB}/include)
    find_library(LEVELDB_LIB leveldb ${WITH_LEVELDB}/lib /usr/local/lib NOTFOUND)
    if (NOT LEVELDB_LIB STREQUAL "LEVELDB_LIB-NOTFOUND")
        target_link_libraries (${TARGET} ${LEVELDB_LIB})
    endif()
endif()
