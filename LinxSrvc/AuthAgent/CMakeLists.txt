cmake_minimum_required (VERSION 3.8)

project ("AuthAgent")

set(TARGET AuthAgent)

add_executable (${TARGET}
	"AuthAgent.cpp"
	"HttpEvent.cpp"
)

include_directories (${WITH_EVENT}/include)

find_library(EVENT_LIB event ${WITH_EVENT}/lib /usr/local/lib NOTFOUND)

target_link_libraries (${TARGET}
	-fPIC
	-Wall
	-ggdb
	${EVENT_LIB}
)

if (DEFINED ENV{WITH_LEVELDB})
	include_directories (${WITH_LEVELDB}/include)
	find_library(LEVELDB_LIB leveldb ${WITH_LEVELDB}/lib /usr/local/lib NOTFOUND)
	if (NOT LEVELDB_LIB STREQUAL "LEVELDB_LIB-NOTFOUND")
		target_link_libraries (${TARGET} ${LEVELDB_LIB})
	endif()
endif()
