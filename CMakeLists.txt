cmake_minimum_required(VERSION 3.5)
project(CUT)

if(WIN32)
    add_custom_target(
        compile1header
        COMMAND ${CMAKE_SOURCE_DIR}/py/generate-win.py ${CMAKE_SOURCE_DIR}/src/core.h ${CMAKE_SOURCE_DIR}/1header/cut.h
    )
    add_custom_target(
        version
        COMMAND ${CMAKE_SOURCE_DIR}/py/generate-win.py ${CMAKE_SOURCE_DIR}/src/core.h ${CMAKE_SOURCE_DIR}/single-header/cut.h --no-comment
    )
else()
    add_custom_target(
        compile1header
        COMMAND ${CMAKE_SOURCE_DIR}/py/generate.py ${CMAKE_SOURCE_DIR}/src/core.h ${CMAKE_SOURCE_DIR}/1header/cut.h
    )
    add_custom_target(
        version
        COMMAND ${CMAKE_SOURCE_DIR}/py/generate.py ${CMAKE_SOURCE_DIR}/src/core.h ${CMAKE_SOURCE_DIR}/single-header/cut.h --no-comment
    )
endif()


add_subdirectory(tests)


