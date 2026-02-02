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
find_package(SQLiteCpp REQUIRED)

qt_standard_project_setup()

if(NOT TARGET chat_proto)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../common
        ${CMAKE_CURRENT_BINARY_DIR}/common
    )
endif()

qt_add_executable(chat_client
    src/main.cpp
    src/ui/main_window.cpp
    src/ui/ban_list_view/ban_list_view.cpp
    src/ui/ban_list_view/banned_users_model.cpp
    src/ui/chat_window.cpp
    src/ui/client_list_helper.cpp
    src/ui/login_view.cpp
    src/ui/private_chat_window.cpp
    src/service/chat_service_grpc.cpp
    src/database/database_manager_sqlite.cpp
    src/ui/res/app.qrc
)

target_include_directories(chat_client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chat_client
    PRIVATE
        Qt6::Widgets
        chat_proto
        SQLiteCpp)
