cmake_minimum_required(VERSION 3.0)

project (toy_memcached)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "-Wall -g")

#include_directories("$(PROJECT_SOURCE_DIR)/Library")
#add_subdirectory(Library)
add_subdirectory(src)
