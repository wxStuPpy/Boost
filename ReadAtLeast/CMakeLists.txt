cmake_minimum_required(VERSION 3.10)
project(main)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++14")

aux_source_directory(. SRC)

add_executable(server ${SRC})

#配置编译选项
