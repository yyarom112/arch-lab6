cmake_minimum_required(VERSION 3.8)
project(lab6)

set(CMAKE_C_STANDARD 99)

set(SOURCE_FILES main.c line_parser.h line_parser.c job_control.c job_control.h )
add_executable(lab6 ${SOURCE_FILES})