cmake_minimum_required(VERSION 2.8.3)
project(tinyxml2)


add_compile_options(-std=c++11)
include_directories( ./ )

## Declare a C++ library
add_library(${PROJECT_NAME} tinyxml2.cpp )


