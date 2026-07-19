<div align="center">

# <img src="Logo.png" alt="KV-Database logo" width="400">

**A lightweight, persistent key-value database built from scratch in C++**

_Redis-inspired · event-driven · cross-platform · containerized_

[![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.15%2B-064F8C?style=flat-square&logo=cmake&logoColor=white)](https://cmake.org/)
[![Docker](https://img.shields.io/badge/Docker-ready-2496ED?style=flat-square&logo=docker&logoColor=white)](https://www.docker.com/)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows-informational?style=flat-square&logo=linux&logoColor=white)](#)
[![Tests](https://img.shields.io/badge/tests-GoogleTest-4285F4?style=flat-square&logo=google&logoColor=white)](https://github.com/google/googletest)
[![License: MIT](https://img.shields.io/badge/license-MIT-brightgreen?style=flat-square)](LICENSE)

</div>

---

## About

kv-db is a Redis-inspired key-value store that supports basic CRUD operations over a TCP connection. It is designed around a phonebook use case where string keys map to string values.

This is a personal learning project. The goal is not to build a production database, but to understand how databases, servers, networking, and concurrency actually work under the hood.

The server runs cross-platform on both **Windows and Linux** using platform-specific networking abstractions.

---

## Table of Contents

- [Features](#features)
- [Commands](#commands)
- [Architecture](#architecture)
- [Concurrency Model](#concurrency-model)
- [Rate Limiting](#rate-limiting)
- [Getting Started](#getting-started)
- [Build Locally](#build-locally)
- [Run With Docker](#run-with-docker)
- [Project Structure](#project-structure)
- [Roadmap](#roadmap)
- [License](#license)

---

## Features

| Feature | Description |
|---|---|
| 🧩 **Custom HashMap** | Hand-rolled hash table with separate chaining for collision resolution and dynamic growth, protected by a `std::shared_mutex` for concurrent read/write safety |
| ⚡ **Event-Driven TCP Server** | Readiness-based event loop supporting many concurrent connections without one thread per client — `epoll` on Linux, IOCP on Windows (adapted via zero-byte `WSARecv`), both behind a shared `IEventLoop` interface selected automatically at compile time |
| 🧵 **Thread Pool** | Fixed pool of 3 worker threads executing actual command work (GET/INSERT/DELETE), decoupled from connection count |
| 📝 **Command Parser** | Parses `INSERT`, `GET`, and `DELETE` commands from raw TCP bytes |
| 🕒 **Snapshot Scheduler** | Background thread that automatically triggers RDB snapshots on a fixed interval |
| 💾 **Hybrid Persistence** | **AOF** logs every write in real time; **RDB** snapshots dump the full database every 5 minutes; AOF resets after each snapshot |
| 🚦 **Rate Limiting** | Per-IP and global request throttling using the Token Bucket algorithm |
| 🛑 **Graceful Shutdown** | Type `stop` to cleanly flush data and join all threads |
| 🐳 **Docker Support** | Multi-stage containerized build targeting Linux |
| 🔐 **Authentication** | _In progress_ |

---

## Commands

Connect to the server using `ncat` or any TCP client:

```bash
ncat 127.0.0.1 6625
```

| Command            | Description                         | Example                 |
| ------------------ | ------------------------------------| ------------------------|
| `INSERT key value` | Inserts or updates a key-value pair | `INSERT Ahmed 51020651` |
| `GET key`          | Retrieves the value for a key       | `GET Ahmed`              |
| `DELETE key`       | Removes a key-value pair            | `DELETE Ahmed`           |

---

## Architecture

```
Client (ncat / custom client)
        │
        │ TCP
        ▼
 Cross-Platform TCP Server
 (Winsock2 / Linux sockets)
        │
        ▼
 Event Loop (readiness-based)
 epoll (Linux) / IOCP (Windows)
 behind a shared IEventLoop interface
        │  watches all client sockets;
        │  reports which ones are ready
        ▼
        ├──▶ Rate Limiter (Token Bucket)
        │    per-IP + global request cap
        │         │
        │         ▼
        │    Command Parser
        │         │
        │         ▼
        │    Thread Pool (3 workers)
        │    executes actual GET/INSERT/DELETE
        │         │
        │         ▼
        │    HashMap (in-memory store)
        │    shared_mutex: readers run concurrently,
        │    writers get exclusive access
        │
        └──▶ Persistence Layer
                  ├── appendonly.log  (real-time AOF log)
                  └── snapshot.log    (periodic RDB snapshot)
                            ▲
                   SnapshotScheduler
                   (background thread, every 5 min)
```

---

## Concurrency Model

| Component    | Protection                               | Strategy                                               |
| ------------ | ----------------------------------------- | -------------------------------------------------------|
| Event Loop   | Single dedicated thread                  | Watches all sockets for readiness; never blocks on I/O  |
| Client jobs  | `ThreadPool` + `std::condition_variable` | Only dispatched once a socket is actually readable      |
| Connections  | `busySockets` guard (`std::mutex`)       | Prevents duplicate recv() jobs for the same socket       |
| HashMap      | `std::shared_mutex`                      | Multiple readers, exclusive writers                      |
| AOF stream   | `std::mutex`                             | Single writer at a time                                  |
| Snapshot     | `SnapshotScheduler` thread               | Sleeps on interval, wakes on shutdown                     |
| Rate limiter | `std::mutex` per map + global            | Per-IP and global window isolated                          |

---

## Rate Limiting

kv-db uses the **Token Bucket** algorithm for rate limiting — the same approach used by Stripe, GitHub, and AWS.

- Each IP gets a bucket of tokens (default: 10)
- Each request consumes one token
- Tokens refill at a fixed rate (default: 5/sec)
- A global cap limits total requests per second across all IPs (default: 1000)
- Blocked requests are dropped immediately without consuming a worker thread

---

## Getting Started

### Prerequisites

- C++17 compiler
- CMake 3.15+
- Docker (optional)
- [nmap/ncat](https://nmap.org/download.html) for testing

For local builds:

- **Windows:** MSVC / MinGW with Winsock2
- **Linux:** GCC / Clang with POSIX sockets

---

## Build Locally

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/Debug/KV_Database.exe   # Windows
./build/KV_Database             # Linux
```

The server starts on port `6625` by default. Type `stop` to shut it down cleanly.

---

## Run With Docker

Build the image:

```bash
docker build -t kv-db:latest .
```

Run the container:

```bash
docker run -d -p 6625:6625 --name my-kv-store kv-db:latest
```

Connect using:

```bash
ncat 127.0.0.1 6625
```

---

## Project Structure

```
kv-db/
├── src/
│   ├── main.cpp
│   ├── Server/
│   │   ├── Server.h
│   │   └── Server.cpp
│   ├── Networking/
│   │   ├── NetworkTypes.h
│   │   ├── EventLoop.h
│   │   ├── EpollEventLoop.h / .cpp
│   │   ├── IocpEventLoop.h / .cpp
│   │   └── EventLoopFactory.h
│   ├── UserSession/
│   │   ├── UserSession.h / .cpp
│   │   └── Connection.h
│   ├── RAM/
│   │   ├── HashMap.h
│   │   └── HashMap.cpp
│   ├── Storage/
│   │   ├── Persistence.h
│   │   └── Persistence.cpp
│   ├── Worker/
│   │   ├── ThreadPool.h
│   │   ├── ThreadPool.cpp
│   │   ├── SnapshotScheduler.h
│   │   └── SnapshotScheduler.cpp
│   ├── Limit/
│   │   ├── Limiter.h
│   │   └── Limiter.cpp
│   └── Tests/
│       ├── test_hashmap.cpp
│       └── test_threadpool.cpp
├── Dockerfile
├── .dockerignore
├── CMakeLists.txt
└── README.md
```

---

## Roadmap

- [x] Custom HashMap with separate chaining
- [x] TCP server with Winsock2
- [x] Cross-platform networking support (Windows/Linux)
- [x] Command parser (INSERT, GET, DELETE)
- [x] AOF persistence with real-time logging
- [x] RDB snapshots every 5 minutes
- [x] AOF reset after each snapshot
- [x] Thread pool for concurrent client sessions
- [x] HashMap thread safety with shared_mutex
- [x] Persistence thread safety with mutex
- [x] Snapshot scheduler background thread
- [x] Clean server shutdown
- [x] Data recovery on restart (RDB + AOF replay)
- [x] Unit tests with GoogleTest (HashMap + ThreadPool)
- [x] Docker containerization (multi-stage Linux build)
- [x] Rate limiting with Token Bucket algorithm
- [x] Cross-platform event-driven architecture (epoll / IOCP)
- [ ] Route idle-session socket cleanup through Server's connection teardown path
- [ ] TCP fragmentation / message framing (LineBuffer)
- [ ] Authentication (username + password)

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

<div align="center">

_Built from scratch as a self-learning project to understand C++, networking, persistence, and systems programming._

</div>