CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.16)

add_definitions(-std=c++11)
set(CXX_STANDARD 11)

SET(PROJ_NAME dpsk_test)
PROJECT(${PROJ_NAME} C CXX)
FILE(GLOB SOURCE_FILES "*.c*")
set(COMPILE_FLAGS "-fPIC -Wall -ggdb")

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
if (${CURL_LIBRARIES} MATCHES "NOTFOUND")
    find_library(CURL_LIBRARIES curl /usr/lib /usr/local/lib /usr/lib/arm-linux-gnueabihf)
endif()
set(EXTERNAL_LIBS ${CURL_LIBRARIES})

find_package(nlohmann_json QUIET)
if (NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
      json
      GIT_REPOSITORY https://github.com/nlohmann/json.git
      GIT_TAG v3.10.5
    )
    FetchContent_MakeAvailable(json)
endif()
list(APPEND EXTERNAL_LIBS nlohmann_json::nlohmann_json)

file(GLOB SRC_FILE_LIST "*[^.h]*.c*[^.txt]*")
add_executable (${PROJ_NAME} ${SRC_FILE_LIST})
target_link_libraries (${PROJ_NAME}
    -pthread
    ${EXTERNAL_LIBS}
)

SET(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)

ADD_CUSTOM_COMMAND(
    TARGET ${PROJ_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${PROJ_NAME} ${EXEC_PATH}
    COMMENT "-- To use ${PROJ_NAME} DO NOT forget to set apiKey by `export DPSK_API_KEY=sk-xxxxxxxx` at first."
)
