cmake_minimum_required(VERSION 3.16)

project(ChatClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

if(NOT TARGET chat_proto)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common
        ${CMAKE_CURRENT_BINARY_DIR}/common
    )
endif()

qt_add_executable(chat_client
    src/main.cpp
    src/chat_window.cpp
    src/grpc_chat_client.cpp
)

target_link_libraries(chat_client 
    PRIVATE 
        Qt6::Widgets 
        chat_proto)
