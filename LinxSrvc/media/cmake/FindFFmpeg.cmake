# -------------- FindFFmpeg.cmake --------------
find_package(PkgConfig QUIET)

pkg_check_modules(PC_FFMPEG QUIET libavcodec libavformat libavutil)

find_path(FFmpeg_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    HINTS ${PC_FFMPEG_INCLUDE_DIRS}
)

find_library(FFmpeg_avcodec_LIBRARY
    NAMES avcodec
    HINTS ${PC_FFMPEG_LIBRARY_DIRS}
)
find_library(FFmpeg_avformat_LIBRARY
    NAMES avformat
    HINTS ${PC_FFMPEG_LIBRARY_DIRS}
)
find_library(FFmpeg_avutil_LIBRARY
    NAMES avutil
    HINTS ${PC_FFMPEG_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS
        FFmpeg_INCLUDE_DIR
        FFmpeg_avcodec_LIBRARY
        FFmpeg_avformat_LIBRARY
        FFmpeg_avutil_LIBRARY
)

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::FFmpeg)
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
    set_target_properties(FFmpeg::FFmpeg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES
            "${FFmpeg_avcodec_LIBRARY};${FFmpeg_avformat_LIBRARY};${FFmpeg_avutil_LIBRARY}"
    )
endif()
# -----------------------------------------------
