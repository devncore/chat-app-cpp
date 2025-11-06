cmake_minimum_required(VERSION 3.16)

project(ChatClient LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

qt_standard_project_setup()

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

set_source_files_properties(${GENERATED_SRCS} ${GENERATED_HDRS} PROPERTIES GENERATED TRUE)

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
    COMMENT "Generating chat client protobuf sources"
    VERBATIM
)

add_custom_target(generate_chat_proto_client DEPENDS ${GENERATED_SRCS} ${GENERATED_HDRS})

qt_add_executable(chat_client
    src/main.cpp
    ${GENERATED_SRCS}
)

add_dependencies(chat_client generate_chat_proto_client)

target_link_libraries(chat_client PRIVATE Qt6::Widgets)
target_link_libraries(chat_client PRIVATE protobuf::libprotobuf gRPC::grpc++)

target_include_directories(chat_client
    PRIVATE
        ${GENERATED_DIR}
)
