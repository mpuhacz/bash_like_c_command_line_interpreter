cmake_minimum_required(VERSION 3.3)
project(SOP2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/builtin.c
    src/builtin.h
    src/helpers.c
    src/helpers.h
    src/main.c
    src/main.h
    src/parser.c
    src/parser.h)

add_executable(SOP2 ${SOURCE_FILES})