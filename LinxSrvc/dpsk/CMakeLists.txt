CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.16)

set(CXX_STANDARD 11)
set(PROJ_NAME dpsk_test)
project(${PROJ_NAME} C CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fPIC -Wall -ggdb")

file(GLOB SRC_FILE_LIST "*[^.h]*.c*[^.txt]*" "*.hpp")

find_package(CURL QUIET)
if (NOT CURL_FOUND)
    find_program(PKG_MANAGER apt-get yum)
    if (PKG_MANAGER)
        execute_process(COMMAND sudo ${PKG_MANAGER} install -y libcurl4-openssl-dev libcurl-devel)
        find_package(CURL QUIET)
    endif()
    if (NOT CURL_FOUND)
        include(ExternalProject)
        ExternalProject_Add(
            curl
            URL https://curl.se/download/curl-7.79.1.tar.gz
            PREFIX ${CMAKE_BINARY_DIR}/curl
            CONFIGURE_COMMAND ./configure --prefix=<INSTALL_DIR>
            BUILD_COMMAND make
            INSTALL_COMMAND make install
        )
        set(CURL_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/curl/include)
        set(CURL_LIBRARIES ${CMAKE_BINARY_DIR}/curl/lib/libcurl.a)
    endif()
endif()

find_package(nlohmann_json QUIET)
if (NOT nlohmann_json_FOUND)
    find_program(PKG_MANAGER apt-get yum)
    if (PKG_MANAGER)
        execute_process(COMMAND sudo ${PKG_MANAGER} install -y nlohmann-json3-dev nlohmann-json-devel)
        find_package(nlohmann_json QUIET)
    endif()
    if (NOT nlohmann_json_FOUND)
        include(FetchContent)
        FetchContent_Declare(
          json
          GIT_REPOSITORY https://github.com/nlohmann/json.git
          GIT_TAG v3.10.5
        )
        FetchContent_MakeAvailable(json)
    endif()
endif()

if (${CURL_LIBRARIES} MATCHES "NOTFOUND")
    find_library(CURL_LIBRARIES curl /usr/lib /usr/local/lib /usr/lib/arm-linux-gnueabihf)
endif()
include_directories(${CURL_INCLUDE_DIRS})
list(APPEND EXTERNAL_LIBS ${CURL_LIBRARIES} nlohmann_json::nlohmann_json)

add_executable(${PROJ_NAME} ${SRC_FILE_LIST})
target_link_libraries(${PROJ_NAME} -pthread ${EXTERNAL_LIBS})

set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)
add_custom_command(
    TARGET ${PROJ_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${PROJ_NAME}> ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/params.txt ${EXEC_PATH}
    COMMENT "-- To use ${PROJ_NAME} DO NOT forget to set apiKey by `export DPSK_API_KEY=sk-xxxxxxxx` at first."
)
