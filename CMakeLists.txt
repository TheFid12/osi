cmake_minimum_required(VERSION 3.16)

project(lab2 C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(lab2
    src/main.c
    src/process.c
    src/process.h
)

target_compile_definitions(lab2 PRIVATE _POSIX_C_SOURCE=200809L)
target_compile_options(lab2 PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror
)
