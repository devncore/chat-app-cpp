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
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common/proto)
set(PROTO_FILE ${PROTO_SRC_DIR}/chat.proto)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${GENERATED_DIR})

set(GENERATED_SRCS
    ${GENERATED_DIR}/chat.pb.cc
    ${GENERATED_DIR}/chat.grpc.pb.cc
)

set(GENERATED_HDRS
    ${GENERATED_DIR}/chat.pb.h
    ${GENERATED_DIR}/chat.grpc.pb.h
)

add_custom_command(
    OUTPUT ${GENERATED_SRCS} ${GENERATED_HDRS}
    COMMAND $<TARGET_FILE:protobuf::protoc>
    ARGS
        --grpc_out=${GENERATED_DIR}
        --cpp_out=${GENERATED_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        -I ${PROTO_SRC_DIR}
        ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating gRPC sources"
    VERBATIM
)

add_custom_target(generate_chat_proto DEPENDS ${GENERATED_SRCS} ${GENERATED_HDRS})

add_library(chat_proto STATIC
    ${GENERATED_SRCS}
    ${GENERATED_HDRS}
)

add_dependencies(chat_proto generate_chat_proto)

target_link_libraries(chat_proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

target_include_directories(chat_proto
    PUBLIC
        ${GENERATED_DIR}
)

add_executable(chat_server
    src/main.cpp
    src/database_manager.cpp
)

target_link_libraries(chat_server
    PRIVATE
        chat_proto
        Qt6::Core
        Qt6::Sql
)
