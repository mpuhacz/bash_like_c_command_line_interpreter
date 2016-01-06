cmake_minimum_required(VERSION 3.3)
project(sop2_university_project)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


include_directories(/usr/include/readline)

set(SOURCE_FILES
    src/builtin.c
    src/builtin.h
    src/helpers.c
    src/helpers.h
    src/main.c
    src/main.h
    src/parser.c
    src/parser.h)

add_executable(sop2_university_project ${SOURCE_FILES})


TARGET_LINK_LIBRARIES(sop2_university_project readline)