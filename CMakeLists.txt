cmake_minimum_required(VERSION 3.18)

project("hsk_sponza_sample")

# Use C++ 20 Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(BASE_FLAGS "${CMAKE_CXX_FLAGS} -fconcepts-ts")
set(NONSTRICT_FLAGS ${BASE_FLAGS})
set(STRICT_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

add_subdirectory("hsk_rt_rpf")
add_subdirectory("src")
