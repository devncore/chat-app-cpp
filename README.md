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
  - **std::jthread/std::mutex**: concurrency primitives.
- Common:
  - **CMake**: portable builds and shared proto generation.
  - **Git**: version control.
  - **VS Code `tasks.json`**: repeatable build/run/tidy/format tasks.
  - **clang-tidy**: static analysis.
  - **clang-format**: formatting consistency.

## Features & Design Patterns

### Server Features
- **Real-time message broadcasting**: Messages are streamed to all connected
  clients with history support for late joiners.
- **Client event broadcasting**: Roster updates (connect/disconnect) are pushed
  to all clients in real-time.
- **Client registry**: Centralized tracking of connected clients with metadata
  (pseudonym, gender, country, connection time).
- **Message validation**: Pluggable validation chain for content rules and rate
  limiting.
- **Database event logging**: Persistent logging of connections, disconnections,
  and message statistics to SQLite.

### Client Features
- **Dual-stream architecture**: Separate gRPC streams for messages and client
  events, each managed on independent threads.
- **Qt signal/slot integration**: Seamless bridge between async gRPC callbacks
  and the Qt event loop.
- **Lazy stub initialization**: gRPC channel and stub created on demand.
- **Thread-safe stream management**: Atomic flags and proper cancellation for
  clean shutdown.

### Design Patterns

| Pattern | Location | Purpose |
|---------|----------|---------|
| **Factory** | `DatabaseManagerFactory` | Encapsulates `DatabaseManagerSQLite` creation with error handling via `std::expected`. Keeps instantiation logic out of `main()`. |
| **Observer** | `ChatServiceEventsDispatcher` | Decouples the gRPC service layer from domain logic. `ChatService` fires events; observers (`ClientRegistry`, `MessageBroadcaster`, `ClientEventBroadcaster`, `DatabaseEventLogger`) react independently. Uses `std::weak_ptr` for safe lifetime management. |
| **Chain of Responsibility** | `MessageValidationChain` | Validators (`ContentValidator`, `RateLimitValidator`) are chained via a fluent API. Each validator can pass or reject; the chain short-circuits on first failure. Easy to extend with new rules. |
| **Callback / Functional** | `ChatServiceGrpc` (client) | gRPC async reads use `std::function` callbacks, which then emit Qt signals to cross the thread boundary safely. |

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
- Unit tests exist for core components (event dispatcher, validation chain).
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
