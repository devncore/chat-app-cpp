# Chat Project

## Overview
A gRPC-based chat system with a Qt client UI and a C++ server that tracks
clients, streams messages, and stores lightweight statistics in SQLite.

## Context
This is a showcase project for recruiters and engineering teams. I use it to
demonstrate and promote my C++ skills.

## Tech Stack Rationale
- Client:
  - **Qt Widgets/Core**: UI and event loop integration.
  - **gRPC**: streaming RPC transport.
- Server:
  - **gRPC**: service layer and streaming endpoints.
  - **SQLiteCpp**: C++ wrapper for SQLite access.
  - **std::thread/std::mutex**: concurrency primitives.
  - **design pattern used**: factory
- Common:
  - **CMake**: portable builds and shared proto generation.
  - **Git**: version control.
  - **VS Code `tasks.json`**: repeatable build/run/tidy/format tasks.
  - **clang-tidy**: static analysis.
  - **clang-format**: formatting consistency.

## Architecture

```
             +--------------------+
             |   Qt Client (UI)   |
             |   ChatWindow       |
             +---------+----------+
                       |
                       | ChatServiceGrpc
                       v
             +---------+----------+
             |  gRPC transport    |
             +---------+----------+
                       |
                       | gRPC (protobuf)
                       v
             +---------+----------+
             |   ChatService      |
             |  gRPC service      |
             +---------+----------+
                       |
                       | ChatRoom (domain)
                       v
             +---------+----------+
             |  In-memory state   |
             |  clients/messages  |
             +---------+----------+
                       |
                       | IDatabaseManager
                       v
             +-----------------------+
             | DatabaseManagerSQLite |
             | SQLiteCpp + SQLite    |
             +-----------------------+
```

## Build Instructions

### Root (recommended)
Build both client and server with the shared protobuf target:

```
cmake -S . -B build
cmake --build build
```

Run:
```
./build/server/chat_server --listen=0.0.0.0:50051
./build/client/chat_client --server=localhost:50051
```

### Client-only
```
cmake -S client -B client/build
cmake --build client/build
./client/build/chat_client --server=localhost:50051
```

### Server-only
```
cmake -S server -B server/build
cmake --build server/build
./server/build/chat_server --listen=0.0.0.0:50051
```

## Naming Conventions
- Files: `lower_snake_case` for `.cpp`/`.hpp`.
- C++ types (classes/structs/enums): `UpperCamelCase`.
- C++ methods/functions: `lowerCamelCase`.
- C++ member variables: `lowerCamelCase_`.
- Protobuf: messages/services `UpperCamelCase`, fields `lower_snake_case`,
  RPCs `UpperCamelCase` verbs, enum values `UPPER_SNAKE_CASE`.

## Testing / CI
- No automated tests or CI are wired yet.
- The `ChatRoom` and `ChatClientSession` interfaces are intended seams for unit
  tests and mocks.
- Doxygen docs can be generated via the VS Code task `generate_doxygen`.

## Known Limitations
- Single global chat room; no channels or private chats.
- No authentication or encryption; gRPC uses insecure credentials by default.
- Client/server lifecycle assumes a stable network and does not persist chat
  history across restarts.

## Notes
- Protobuf generation is centralized in `common/` and linked by both client and
  server to avoid drift.
- CLI flags control endpoints: `--server` for client, `--listen` for server.
