cmake_minimum_required(VERSION 2.8.3)
project(abseil_cpp)


add_compile_options(-std=c++11)
include_directories( ./ ./cctz/include)

file(GLOB_RECURSE source_list "*.cc" )

list(REMOVE_ITEM source_list ${CMAKE_CURRENT_SOURCE_DIR}/absl/synchronization/internal/mutex_nonprod.cc )

## Declare a C++ library
add_library(${PROJECT_NAME}  ${source_list} )


