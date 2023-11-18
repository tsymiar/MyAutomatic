SET(cmake_generated ${CMAKE_BINARY_DIR}/CMakeCache.txt
                    ${CMAKE_BINARY_DIR}/cmake-install.cmake
                    ${CMAKE_BINARY_DIR}/Makefile
                    ${CMAKE_BINARY_DIR}/CMakeFiles
)
FOREACH(file ${cmake_generated})
    IF(EXISTS ${file})
        FILE(REMOVE_RECURSE ${file})
    ENDIF()
ENDFOREACH()
