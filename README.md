# Chat Project

A real-time chat application built with **C++23**, **gRPC** bidirectional
streaming, **Qt 6 Widgets**, and **SQLite**. Features public/private messaging,
live roster updates, and per-user ban lists.

## Context

This is a showcase project for recruiters and engineering teams. I use it to
demonstrate my C++ and Qt skills through a non-trivial, multi-component
application.

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
                       | Protobuf (streaming)
                       v
             +---------+----------+
             |   ChatService      |
             |  gRPC service      |
             +---------+----------+
                       |
                       | Domain layer
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

## Project Structure

```
chat/
  client/
    src/
      ui/           MainWindow, ChatWindow, LoginView, PrivateChatWindow,
                    BanListView, BannedUsersModel
      service/      ChatServiceGrpc (gRPC client adapter)
      database/     IDatabaseManager, DatabaseManagerSQLite
  server/
    src/
      service/      ChatService (gRPC service implementation)
      domain/       ClientRegistry, MessageBroadcaster, PrivateMessageBroadcaster,
                    ClientEventBroadcaster
      database/     DatabaseManagerSQLite (event logging)
      grpc/         GrpcRunner (server lifecycle)
    tests/          Unit tests (event dispatcher, validation chain)
  common/
    proto/          chat.proto (shared service & message definitions)
```

## Tech Stack

| Layer | Technology | Role |
|-------|-----------|------|
| **Client** | Qt 6 Widgets | UI and event loop |
| **Client** | gRPC | Streaming RPC transport |
| **Client** | SQLiteCpp | Per-user ban list persistence |
| **Server** | gRPC | Service layer and streaming endpoints |
| **Server** | SQLiteCpp | Connection/message statistics |
| **Server** | Boost.ProgramOptions | CLI argument parsing |
| **Common** | CMake | Build system and shared proto generation |
| **Common** | Protobuf | Wire format and service definition |
| **Tooling** | clang-tidy | Static analysis |
| **Tooling** | clang-format | Code formatting |
| **Tooling** | Doxygen | API documentation generation |
| **Tooling** | ccache | Compilation caching |

## Qt Concepts Showcase (Client)

A curated list of Qt features and patterns demonstrated in this project.

- **Signal & Slot mechanism** -- type-safe, cross-object communication wired
  throughout the app
  ([main.cpp](client/src/main.cpp),
  [chat_window.cpp](client/src/ui/chat_window.cpp))
- **QMainWindow** -- top-level window with dock widget and toolbar integration
  ([main_window.hpp](client/src/ui/main_window.hpp))
- **QDockWidget** -- dockable login panel attached to the main window
  ([main_window.cpp](client/src/ui/main_window.cpp))
- **QToolBar & QAction** -- toolbar with disconnect action and SVG icon
  ([main_window.cpp](client/src/ui/main_window.cpp))
- **Qt Resource System (.qrc)** -- icons are bundled via a `.qrc` file and
  loaded with `:/` resource paths instead of filesystem paths, the idiomatic
  Qt way to embed assets into the binary
  ([app.qrc](client/src/ui/res/app.qrc))
- **QWidget as standalone window** -- `PrivateChatWindow` uses `Qt::Window`
  flag to spawn independent top-level windows
  ([private_chat_window.cpp](client/src/ui/private_chat_window.cpp))
- **Context menu (QMenu)** -- right-click menu on the user list for private
  messaging and ban/unban actions
  ([chat_window.cpp](client/src/ui/chat_window.cpp))
- **External CSS stylesheet** -- application-wide theming loaded from an
  external `.css` file via `QApplication::setStyleSheet()`
  ([style.css](client/src/ui/style.css),
  [main.cpp](client/src/main.cpp))
- **Layout management** -- `QVBoxLayout`, `QHBoxLayout`, and `QFormLayout`
  used for programmatic UI composition (no `.ui` files)
  ([login_view.cpp](client/src/ui/login_view.cpp),
  [chat_window.cpp](client/src/ui/chat_window.cpp))
- **Rich text rendering** -- HTML-formatted messages with color-coded authors
  displayed in `QTextBrowser`
  ([chat_window.cpp](client/src/ui/chat_window.cpp))
- **QListWidget & item roles** -- user roster with `Qt::UserRole` data
  storage for banned-state tracking
  ([chat_window.cpp](client/src/ui/chat_window.cpp))
- **Model/View with QAbstractListModel** -- `BannedUsersModel` subclasses
  `QAbstractListModel` and drives a `QListView`, separating data from
  presentation
  ([banned_users_model.hpp](client/src/ui/ban_list_view/banned_users_model.hpp),
  [ban_list_view.cpp](client/src/ui/ban_list_view/ban_list_view.cpp))
- **QPointer** -- prevent dangling pointers to private chat windows
  ([chat_window.hpp](client/src/ui/chat_window.hpp))
- **QCommandLineParser** -- CLI argument parsing for the `--server` option
  ([main.cpp](client/src/main.cpp))
- **QMessageBox** -- modal dialogs for errors and warnings
  ([login_view.cpp](client/src/ui/login_view.cpp),
  [main_window.cpp](client/src/ui/main_window.cpp))
- **closeEvent() override** -- custom window-close handling for graceful
  stream shutdown
  ([main_window.cpp](client/src/ui/main_window.cpp))
- **Lambda slots** -- inline lambdas connected to signals for concise
  one-liner handlers
  ([chat_window.cpp](client/src/ui/chat_window.cpp))
- **Cross-thread signal emission** -- gRPC callbacks on background threads
  emit Qt signals to safely update the UI on the main thread
  ([chat_service_grpc.cpp](client/src/service/chat_service_grpc.cpp))
- **AUTOMOC / AUTOUIC / AUTORCC** -- CMake-driven meta-object compilation
  ([CMakeLists.txt](client/CMakeLists.txt))

## Features

### Server
- Real-time message broadcasting with history for late joiners
- Client event streaming (connect/disconnect roster updates)
- Private message routing between individual clients
- Pluggable message validation chain (content rules, rate limiting)
- Persistent logging of connections and message statistics to SQLite
- Centralized client registry with metadata (pseudonym, gender, country)

### Client
- Public and private messaging with dedicated chat windows
- Dual gRPC streams (messages + client events) on independent threads
- Per-user SQLite database for ban list persistence
- Live user roster with right-click context menu (private message, ban/unban)
- External CSS theming
- Command-line server address configuration

## Design Patterns

| Pattern | Location | Purpose |
|---------|----------|---------|
| **Factory** | `DatabaseManagerFactory` | Encapsulates `DatabaseManagerSQLite` creation with `std::expected` error handling. |
| **Observer** | `ChatServiceEventsDispatcher` | Decouples the gRPC service layer from domain logic. `ChatService` fires events; observers (`ClientRegistry`, `MessageBroadcaster`, `ClientEventBroadcaster`, `DatabaseEventLogger`) react independently via `std::weak_ptr`. |
| **Chain of Responsibility** | `MessageValidationChain` | Validators (`ContentValidator`, `RateLimitValidator`) are chained via a fluent API. Short-circuits on first failure. |
| **Callback / Functional** | `ChatServiceGrpc` | gRPC async reads use `std::function` callbacks that emit Qt signals to cross the thread boundary safely. |

## Build Instructions

### Full build (recommended)

Build both client and server with the shared protobuf target:

```bash
cmake -S . -B build
cmake --build build
```

Run:
```bash
./build/server/chat_server --listen=0.0.0.0:50051
./build/client/chat_client --server=localhost:50051
```

### Client only
```bash
cmake -S client -B client/build
cmake --build client/build
./client/build/chat_client --server=localhost:50051
```

### Server only
```bash
cmake -S server -B server/build
cmake --build server/build
./server/build/chat_server --listen=0.0.0.0:50051
```

### Server tests
```bash
cmake -S server -B server/build -DBUILD_TESTS=ON
cmake --build server/build
ctest --test-dir server/build
```

## Naming Conventions

| Element | Style | Example |
|---------|-------|---------|
| Files | `lower_snake_case.cpp/.hpp` | `chat_window.cpp` |
| Classes / Structs / Enums | `UpperCamelCase` | `ChatServiceGrpc` |
| Methods / Functions | `lowerCamelCase` | `startMessageStream()` |
| Member variables | `lowerCamelCase_` | `messageStreamRunning_` |
| Protobuf messages | `UpperCamelCase` | `ConnectRequest` |
| Protobuf fields | `lower_snake_case` | `connected_pseudonyms` |
| Protobuf RPCs | `UpperCamelCase` | `SubscribeMessages` |
| Enum values | `UPPER_SNAKE_CASE` | `ADD`, `REMOVE` |

## Known Limitations

- Single global chat room; no channel support.
- No authentication or encryption (gRPC insecure credentials).
- Chat history is not persisted across restarts.
- Assumes a stable network connection.
