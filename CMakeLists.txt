cmake_minimum_required(VERSION 3.10)
project(ESCA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE DEBUG)

add_compile_definitions(DEBUG)

#add_compile_definitions(SAVEXML)
#add_compile_definitions(DEBUGOUT)

add_subdirectory(src)

add_executable(test resource/1.cpp)
add_executable(test_me test.cpp)