find_package(GTest REQUIRED)

enable_testing()

add_executable(chat_server_tests
    # Domain tests
    domain/client_registry_test.cpp
    domain/message_broadcaster_test.cpp
    domain/client_event_broadcaster_test.cpp

    # Events tests
    events/event_dispatcher_test.cpp

    # Database tests
    database/database_event_logger_test.cpp

    # Source files under test
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/domain/client_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/domain/message_broadcaster.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/domain/client_event_broadcaster.cpp
)

target_include_directories(chat_server_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(chat_server_tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
        chat_proto
)

include(GoogleTest)
gtest_discover_tests(chat_server_tests)
