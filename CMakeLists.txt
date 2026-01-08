cmake_minimum_required(VERSION 3.16)

project(ChatApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
