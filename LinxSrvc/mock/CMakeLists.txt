CMAKE_MINIMUM_REQUIRED(VERSION 2.8...3.16)

SET(PROJ_NAME trans)
PROJECT(${PROJ_NAME} C CXX)
FILE(GLOB SOURCE_FILES "*.c*" "*/*.c*")
SET(TARGETS server client)

foreach(target IN LISTS TARGETS)
    set(TARGET_NAME "${PROJ_NAME}_${target}")
    add_executable(${TARGET_NAME} ${SOURCE_FILES})
    if(target STREQUAL "client")
        target_compile_definitions(${TARGET_NAME} PRIVATE CLIENT)
    endif()
    IF (WIN32)
        IF (${CMAKE_BUILD_TYPE} STREQUAL "Release")
            SET(_TARGET_ "${CMAKE_BUILD_TYPE}/${TARGET_NAME}")
        ELSE()
            SET(_TARGET_ "${CMAKE_BUILD_TYPE}/../${TARGET_NAME}")
        ENDIF()
        SET(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../out)
    ELSE()
        SET(_TARGET_ ${TARGET_NAME})
        SET(EXEC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../gen)
    ENDIF()
    TARGET_LINk_LIBRARIES(${TARGET_NAME} pthread)
    ADD_CUSTOM_COMMAND(
        TARGET ${TARGET_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${EXEC_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_BINARY_DIR}/${_TARGET_} ${EXEC_PATH}
    )
endforeach()
