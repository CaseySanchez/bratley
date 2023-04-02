cmake_minimum_required(VERSION 3.14)

project(bratley)

find_package(Threads REQUIRED)

add_executable(bratley main.cpp)

target_link_libraries(bratley Threads::Threads)

set_property(TARGET bratley PROPERTY CXX_STANDARD 20)