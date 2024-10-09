cmake_minimum_required(VERSION 3.17)

project(focus-stack)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(PROJECT_LIB ${PROJECT_NAME}_lib)
set(PROJECT_TEST ${PROJECT_NAME}_test)

option(BUILD_TESTS "build tests" OFF) # set to ON if you want to build tests

file(GLOB_RECURSE PROJECT_LIB_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${SRC_DIR}/*.cc ${SRC_DIR}/*.hh ${SRC_DIR}/*cl
)
list(FILTER PROJECT_LIB_SOURCES EXCLUDE REGEX .*_tests.cc|.*main.cc)

add_library(${PROJECT_LIB}
    STATIC # set to SHARED if you want dll
    ${PROJECT_LIB_SOURCES}
)
target_link_libraries(${PROJECT_LIB}
    PUBLIC ${OpenCV_LIBS}
)
if(MSVC)
    target_compile_definitions(${PROJECT_LIB} PRIVATE _USE_MATH_DEFINES)
endif()

add_executable(${PROJECT_NAME} ${SRC_DIR}/main.cc)
target_link_libraries(${PROJECT_NAME}
    PRIVATE ${PROJECT_LIB}
)

if (BUILD_TESTS)
    enable_testing()
    find_package(GTest CONFIG REQUIRED)
    file(GLOB_RECURSE PROJECT_TEST_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
        ${SRC_DIR}/*_tests.cc
    )
    add_executable(${PROJECT_TEST} ${PROJECT_TEST_SOURCES} ${SRC_DIR}/gtest_main.cc)
    target_link_libraries(${PROJECT_TEST}
        PRIVATE ${PROJECT_LIB}
        PRIVATE GTest::gtest_main
    )
    add_test(NAME ${PROJECT_TEST} COMMAND ${PROJECT_TEST})
endif()
