cmake_minimum_required(VERSION 3.16)
project(tsh C)

set(CMAKE_C_STANDARD 99)
find_package (Threads)

add_executable(tsh main.c)
target_link_libraries (tsh ${CMAKE_THREAD_LIBS_INIT})
