cmake_minimum_required(VERSION 3.16)

project(ChatApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
