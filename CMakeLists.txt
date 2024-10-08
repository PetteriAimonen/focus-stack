cmake_minimum_required(VERSION 3.17)

project(focus-stack)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
# find_package(OpenCL CONFIG REQUIRED)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
include_directories(${SRC_INCLUDE_DIR})

file(GLOB_RECURSE PROJECT_SOURCES LIST_DIRECTORIES false CONFIGURE_DEPENDS
    ${SRC_DIR}/*.cc ${SRC_DIR}/*.hh ${SRC_DIR}/*cl
)
list(FILTER PROJECT_SOURCES EXCLUDE REGEX .*_tests.cc|.*test.*)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
target_link_libraries(${PROJECT_NAME}
    PRIVATE ${OpenCV_LIBS}
#    PRIVATE OpenCL::OpenCL
)
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _USE_MATH_DEFINES)
endif()
