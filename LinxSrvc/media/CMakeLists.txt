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
    # -fsanitize=address
)

if(JETSON_TARGET)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(PkgConfig)
    find_package(Threads)
    find_package(OpenGL)
    pkg_check_modules(EGL egl)
    pkg_check_modules(GBM REQUIRED gbm)
    pkg_check_modules(DRM REQUIRED libdrm)
    pkg_check_modules(EGL REQUIRED egl)
    pkg_check_modules(GLESv2 REQUIRED glesv2)
    if(NOT OpenGL_FOUND OR NOT EGL_FOUND)
        if(UNIX AND NOT APPLE)
            execute_process(COMMAND which apt-get RESULT_VARIABLE APT_GET_FOUND)
            if(APT_GET_FOUND EQUAL 0)
                message(STATUS "Installing missing packages...")
                execute_process(COMMAND
                    sudo apt-get install -y
                    libegl1-mesa-dev
                    libopengl-dev
                )
            endif()
        endif()
    endif()
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
            ${GBM_INCLUDE_DIRS}
            ${DRM_INCLUDE_DIRS}
            ${EGL_INCLUDE_DIRS}
            ${GLESv2_INCLUDE_DIRS}
        )
    link_directories(
        /usr/lib/aarch64-linux-gnu/nvidia
        /usr/lib/aarch64-linux-gnu/libv4l/plugins/nv
        ${GBM_LIBRARY_DIRS}
        ${DRM_LIBRARY_DIRS}
    )
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
        EGL
        OpenGL::GL
        ${GBM_LIBRARIES}
        ${DRM_LIBRARIES}
        ${EGL_LIBRARIES}
        ${GLESv2_LIBRARIES}
        dl
    )
else()
    execute_process(
        COMMAND uname -r
        OUTPUT_VARIABLE KERNEL_VERSION_RAW
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message(STATUS "Raw kernel version: ${KERNEL_VERSION_RAW}")
    string(REGEX MATCH "^([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${KERNEL_VERSION_RAW})
    set(KERNEL_MAJOR ${CMAKE_MATCH_1})
    set(KERNEL_MINOR ${CMAKE_MATCH_2})
    set(KERNEL_PATCH ${CMAKE_MATCH_3})
    set(TARGET_MAJOR 6)
    set(TARGET_MINOR 0)
    set(TARGET_PATCH 0)
    if(
        (KERNEL_MAJOR GREATER TARGET_MAJOR) OR
        (KERNEL_MAJOR EQUAL TARGET_MAJOR AND KERNEL_MINOR GREATER TARGET_MINOR) OR
        (KERNEL_MAJOR EQUAL TARGET_MAJOR AND KERNEL_MINOR EQUAL TARGET_MINOR AND KERNEL_PATCH GREATER_EQUAL TARGET_PATCH)
    )        
        message(WARNING "❌ Current kernel version >= ${TARGET_MAJOR}.${TARGET_MINOR}.${TARGET_PATCH}")
        RETURN(${PROJECT_NAME})
    else()
        message(STATUS "✅ Building with supports target kernel V:${TARGET_MAJOR}.${TARGET_MINOR}.${TARGET_PATCH}")
    endif()
    # FFmpeg
    find_package(Threads REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(AVCODEC libavcodec)
    pkg_check_modules(AVFORMAT libavformat)
    pkg_check_modules(AVUTIL libavutil)
    pkg_check_modules(SWSCALE libswscale)
    pkg_check_modules(SWRESAMPLE libswresample)
    execute_process(COMMAND which apt-get RESULT_VARIABLE APT_GET_FOUND)
    if(APT_GET_FOUND EQUAL 0)
        execute_process(COMMAND 
            sudo apt-get install -y
            ffmpeg
            libavcodec-dev
            libavformat-dev
            libavutil-dev
            libswscale-dev
            libswresample-dev
            libsdl2-dev
        )
    endif()
    find_package(SDL2)
    if (NOT SDL2_FOUND)
        message("## SDL2 Not found, skip [${PROJECT_NAME}] building")
        return()
    endif()
    include_directories(
        ${AVCODEC_INCLUDE_DIRS}
        ${AVFORMAT_INCLUDE_DIRS}
        ${AVUTIL_INCLUDE_DIRS}
        ${SWSCALE_INCLUDE_DIRS}
        ${SWRESAMPLE_INCLUDE_DIRS}
        ${SDL2_INCLUDE_DIRS}
    )
    link_directories(${AVCODEC_LIBRARY_DIRS})

    list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    find_package(FFmpeg REQUIRED)
    include_directories(${FFmpeg_INCLUDE_DIRS})

    set(EXTRA_LIBRARIES
        ${CMAKE_THREAD_LIBS_INIT}
        ${AVCODEC_LIBRARIES}
        ${AVFORMAT_LIBRARIES}
        ${AVUTIL_LIBRARIES}
        ${SWSCALE_LIBRARIES}
        ${SWRESAMPLE_LIBRARIES}
        ${SDL2_LIBRARIES}
        Threads::Threads
        ${FFmpeg_LIBRARIES}
    )
endif()

file(GLOB SRC_FILE_LIST "*[^.h]*.c*[^.txt]*" "*.hpp" "*.c")
add_executable(${PROJECT_NAME} ${SRC_FILE_LIST})

target_link_libraries(${PROJECT_NAME}
    ${EXTRA_LIBRARIES}
)

set(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${EXEC_PATH}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/check.sh ${EXEC_PATH}
)
