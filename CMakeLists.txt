cmake_minimum_required(VERSION 3.16)

project(ChatApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Fetch better-enums (header-only library)
include(FetchContent)
FetchContent_Declare(
    better_enums
    GIT_REPOSITORY https://github.com/aantron/better-enums.git
    GIT_TAG 0.11.3
)
FetchContent_MakeAvailable(better_enums)

# Create an interface library for better-enums
add_library(better_enums INTERFACE)
target_include_directories(better_enums INTERFACE ${better_enums_SOURCE_DIR})

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)
