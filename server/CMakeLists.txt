cmake_minimum_required(VERSION 3.16)

project(ChatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Sql)

if(NOT TARGET chat_proto)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common
        ${CMAKE_CURRENT_BINARY_DIR}/common
    )
endif()

add_executable(chat_server
    src/main.cpp
    src/database_manager.cpp
    src/domain/chat_room.cpp
    src/service/chat_service_impl.cpp
)

target_include_directories(chat_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(chat_server
    PRIVATE
        chat_proto
        Qt6::Core
        Qt6::Sql
)
