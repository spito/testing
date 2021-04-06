cmake_minimum_required(VERSION 3.5)
project(CUT)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

add_custom_target(
    compile1header
    COMMAND python3 ${CMAKE_SOURCE_DIR}/py/generate.py ${CMAKE_SOURCE_DIR}/src/core.h ${CMAKE_SOURCE_DIR}/1header/cut.h
)
add_custom_target(
    version
    COMMAND python3 ${CMAKE_SOURCE_DIR}/py/generate.py ${CMAKE_SOURCE_DIR}/src/core.h ${CMAKE_SOURCE_DIR}/single-header/cut.h --no-comment
)

add_subdirectory(tests)


