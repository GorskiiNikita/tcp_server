cmake_minimum_required(VERSION 3.23)
project(tcp_server C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS -pthread)

add_executable(tcp_server main.c)
