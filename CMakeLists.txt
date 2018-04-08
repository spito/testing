cmake_minimum_required(VERSION 3.5)
project(CUT)

add_custom_target(
    compile1header
    COMMAND ${CMAKE_SOURCE_DIR}/py/1hc.py ${CMAKE_SOURCE_DIR}/src/core.h ${CMAKE_SOURCE_DIR}/1header/cut.h
)

add_subdirectory(tests)


