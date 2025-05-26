cmake_minimum_required(VERSION 3.10)

project(autorestool VERSION 1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(autorestool main.cpp version.rc)
target_link_options(autorestool PRIVATE /SUBSYSTEM:windows)
