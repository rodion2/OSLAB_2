set(CMAKE_BUILD_TYPE Debug)

add_library( mmemory-lib mmemory.h mmemory.cpp)

target_include_directories(mmemory-lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


