cmake_minimum_required(VERSION 3.16)

project(ChatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

find_package(Boost REQUIRED COMPONENTS program_options)
find_package(SQLiteCpp REQUIRED)

if(NOT TARGET chat_proto)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common
        ${CMAKE_CURRENT_BINARY_DIR}/common
    )
endif()

add_executable(chat_server
    src/main.cpp
    src/database/database_manager_sqlite.cpp
    src/domain/client_registry.cpp
    src/domain/message_broadcaster.cpp
    src/domain/client_event_broadcaster.cpp
    src/domain/private_message_broadcaster.cpp
    src/grpc/grpc_runner.cpp
    src/service/chat_service.cpp
)

target_include_directories(chat_server
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(chat_server
    PRIVATE
        chat_proto
        Boost::program_options
        SQLiteCpp
        better_enums
)

# Unit tests (build with -DBUILD_TESTS=ON)
option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
