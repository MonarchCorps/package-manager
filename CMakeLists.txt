cmake_minimum_required(VERSION 4.2)
project(package_manager C)

set(CMAKE_C_STANDARD 99)

add_executable(package_manager main.c
        manager.c
        manager.h)
