
include_directories(AFTER SYSTEM "${CMAKE_SOURCE_DIR}/1header")

if(MSVC)
else()
    add_definitions(-DCUT -Wall -Wextra -pedantic -ggdb3)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(main-cpp main.cpp)
add_dependencies(main-cpp compile1header)
add_library(main-c main.c)
add_dependencies(main-c compile1header)


file(GLOB TESTS t-*.c t-*.cpp)
list(APPEND TESTS "")

foreach(TEST_FILE ${TESTS}) 
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    get_filename_component(TEST_TYPE ${TEST_FILE} EXT)
    if (${TEST_DEP})
        unset(TEST_DEP)
    endif()
    if ("${TEST_TYPE}" STREQUAL ".c")
        set(TEST_DEP "main-c")
    elseif ("${TEST_TYPE}" STREQUAL ".cpp")
        set(TEST_DEP "main-cpp")
    else()
        message(SEND_ERROR "Invalid test file ${TEST_FILE} ${TEST_TYPE}")
    endif()

    if (TEST_DEP)
        list(APPEND TESTS ${TEST_NAME})
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE ${TEST_DEP})
    endif()
endforeach()


add_custom_target(
    check
    COMMAND "${CMAKE_SOURCE_DIR}/py/check.py" "${CMAKE_BINARY_DIR}/tests" "${CMAKE_SOURCE_DIR}/tests"
    DEPENDS ${TESTS}
)


