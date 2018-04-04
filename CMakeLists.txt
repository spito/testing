cmake_minimum_required(VERSION 3.5)
project(CUT)

add_custom_target(
    compile1header
    ALL
    COMMAND cd ${CMAKE_SOURCE_DIR} && py/1hc.py src/core.h 1header/cut.h
)

add_subdirectory(tests)


