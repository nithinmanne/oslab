cmake_minimum_required(VERSION 3.9)
project(oslab)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG=true")

add_executable(template template.cpp)
add_executable(1a 1/Ass1_51_1a.cpp)
add_executable(1b 1/Ass1_51_1b.cpp)
add_executable(2 2/Ass2_51.cpp)
add_executable(2a 2/Ass2a_51.cpp)
add_executable(3a 3/Ass3a_51.cpp)
add_executable(3b 3/Ass3b_51.cpp)
add_executable(4 4/Ass4_51.cpp 4/mrfs.h 4/mrfs.cpp)