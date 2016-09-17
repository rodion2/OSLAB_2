cmake_minimum_required(VERSION 3.6)
project(OSLab_2)

set(BOOST_LIBRARYDIR  /usr/include/boost)
set(Boost_DEBUG        ON)
set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)

find_package(Boost 1.54.0 COMPONENTS test REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

set(SOURCE_FILES first_iteration_unit_tests.cpp mmemory.h)
add_executable(OSLab_2 ${SOURCE_FILES})

target_link_libraries(OSLab_2 ${Boost_INCLUDE_DIRS})