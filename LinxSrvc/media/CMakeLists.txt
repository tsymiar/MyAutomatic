cmake_minimum_required(VERSION 2.8...3.16)
project(video_render LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# check if Jetson platform
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" AND EXISTS "/etc/nv_tegra_release")
    set(JETSON_TARGET TRUE)
    message(STATUS "Detected Jetson Platform (Orin Nano)")
    file(STRINGS "/etc/nv_tegra_release" TEGRA_VERSION 
         REGEX "^# R[0-9]+ \\(.*\\)")
    string(REGEX MATCH "R(32|34|35|36)" JP_VERSION ${TEGRA_VERSION})
    if(JP_VERSION STREQUAL "R36")
        set(JP_VERSION 6.2)
    endif()
    message(STATUS "JetPack Version: ${JP_VERSION}")
endif()

add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
    -ggdb
    -pthread
)

find_package(OpenGL)
find_package(glfw3)
find_package(GLEW)
find_package(PkgConfig)
pkg_check_modules(EGL egl)
find_package(X11)

if(NOT OpenGL_FOUND OR NOT glfw3_FOUND OR NOT GLEW_FOUND OR NOT EGL_FOUND OR NOT X11_FOUND)
    if(UNIX AND NOT APPLE)
        execute_process(COMMAND which apt-get RESULT_VARIABLE APT_GET_FOUND)
        if(APT_GET_FOUND EQUAL 0)
            message(STATUS "Installing missing packages...")
            execute_process(COMMAND 
                sudo apt-get install -y
                libglfw3-dev
                libglew-dev
                libegl1-mesa-dev
                libx11-dev
                libopengl-dev
            )
        endif()
    endif()
endif()

if(JETSON_TARGET)
    add_definitions(-DUSE_JETSON_MULTIMEDIA_API)
    # Jetson Multimedia API
    set(NVIDIA_LIB_DIR "/usr/lib/aarch64-linux-gnu/nvidia")
    set(V4L2_PLUGINS_DIR "/usr/lib/aarch64-linux-gnu/libv4l/plugins/nv")
    set(GST_PLUGINS_DIR "/usr/lib/aarch64-linux-gnu/gstreamer-1.0")

    find_library(NVMEDIA_LIB nvmedia PATHS ${NVIDIA_LIB_DIR} REQUIRED)
    find_library(NVBUF_SURFACE_LIB nvbufsurface PATHS ${NVIDIA_LIB_DIR} REQUIRED)
    find_library(NVVIDEO_LIB nvvideo PATHS ${NVIDIA_LIB_DIR} REQUIRED)
    find_library(V4L2_NVARGUS_LIB v4l2_nvargus PATHS ${V4L2_PLUGINS_DIR} REQUIRED)
    find_library(GST_NVCODEC_LIB gstnvcodec PATHS ${GST_PLUGINS_DIR} REQUIRED)
    find_library(NVTRANSFORM_LIB nvbufsurftransform PATHS ${NVIDIA_LIB_DIR} REQUIRED)
    include_directories(
        /etc/alternatives/cuda/targets/aarch64-linux/include
        /usr/include/gstreamer-1.0
        /usr/src/jetson_multimedia_api/include
        /usr/src/jetson_multimedia_api/include/libjpeg-8b
    )
    link_directories(/usr/lib/aarch64-linux-gnu/nvidia
        .. 
       /usr/lib/aarch64-linux-gnu/libv4l/plugins/nv)
    set(EXTRA_LIBRARIES
        ${NVBUF_SURFACE_LIB}
        ${NVVIDEO_LIB}
        ${V4L2_NVARGUS_LIB}
        ${GST_NVCODEC_LIB}
        ${NVTRANSFORM_LIB}
        ${NVMEDIA_LIB}
        nvosd
        cuda
        v4l2
        gstreamer-1.0
        v4l2_nvvideocodec
        GLEW::GLEW
        OpenGL::GL
        glfw
    )
else()
    # FFmpeg
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVCODEC libavcodec)
    pkg_check_modules(AVFORMAT libavformat)
    pkg_check_modules(AVUTIL libavutil)
    pkg_check_modules(SWSCALE libswscale)
    execute_process(COMMAND which apt-get RESULT_VARIABLE APT_GET_FOUND)
    if(APT_GET_FOUND EQUAL 0)
        execute_process(COMMAND 
            sudo apt-get install -y
            ffmpeg
            libavcodec-dev
            libavformat-dev
            libswscale-dev
        )
    endif()
    include_directories(
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
        ${SWSCALE_INCLUDE_DIRS}
    )
    set(EXTRA_LIBRARIES pthread
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
    )
    if(TARGET OpenGL::GL)
        set(OpenGL_LIBRARIES OpenGL::GL)
    endif()
    if (NOT GLEW_LIBRARY)
        message("## GLEW not found, skip build ${PROJECT_NAME}")
        return()
    endif()
endif()

file(GLOB SRC_FILE_LIST "*[^.h]*.c*[^.txt]*" "*.hpp" "*.c")
add_executable(${PROJECT_NAME} ${SRC_FILE_LIST})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenGL_INCLUDE_DIRS}
    ${glfw3_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME}
    ${EGL_LIBRARIES}
    ${X11_LIBRARIES}
    ${OpenGL_LIBRARIES}
    ${glfw3_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${EXTRA_LIBRARIES}
)

set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/setWatch.sh ${EXEC_PATH}
)
