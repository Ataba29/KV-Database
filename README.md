# kv-db

A lightweight, persistent key-value database built from scratch in C++, containerized with Docker. This project was built as a self-learning exercise to deeply understand systems programming, networking, persistence, and concurrency in C++.

---

## About

kv-db is a Redis-inspired key-value store that supports basic CRUD operations over a TCP connection. It is designed around a phonebook use case where string keys map to string values.

This is a personal learning project. The goal is not to build a production database, but to understand how databases, servers, networking, and concurrency actually work under the hood.

The server is designed to run cross-platform on both **Windows and Linux** using platform-specific networking abstractions.

---

## Features

- **Custom HashMap** — hand-rolled hash table with separate chaining for collision resolution and dynamic growth, protected by a `std::shared_mutex` for concurrent read/write safety
- **Cross-Platform TCP Server** — raw socket server supporting:
  - Windows networking through Winsock2
  - Linux networking through POSIX sockets

- **Command Parser** — parses `INSERT`, `GET`, and `DELETE` commands from raw TCP bytes
- **Thread Pool** — fixed pool of 3 worker threads handling client sessions concurrently using condition variables and mutexes
- **Snapshot Scheduler** — background thread that automatically triggers RDB snapshots on a fixed interval
- **Persistence** — hybrid durability layer combining:
  - **AOF (Append-Only File)** — logs every write operation in real time, reset after each snapshot
  - **RDB Snapshots** — full point-in-time dumps of the database every 5 minutes

- **Graceful Shutdown** — type `stop` to cleanly flush data and join all threads
- **Docker Support** — containerized application with Dockerfile and `.dockerignore`
- **Rate Limiting** — _(in progress)_
- **Authentication** — _(in progress)_

---

## Commands

Connect to the server using `ncat` or any TCP client:

```bash
ncat 127.0.0.1 6625
```

| Command            | Description                         | Example                 |
| ------------------ | ----------------------------------- | ----------------------- |
| `INSERT key value` | Inserts or updates a key-value pair | `INSERT Ahmed 51020651` |
| `GET key`          | Retrieves the value for a key       | `GET Ahmed`             |
| `DELETE key`       | Removes a key-value pair            | `DELETE Ahmed`          |

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
        ├──▶ Command Parser
        │         │
        │         ▼
        │    Thread Pool (3 workers)
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

| Component   | Protection                               | Strategy                              |
| ----------- | ---------------------------------------- | ------------------------------------- |
| HashMap     | `std::shared_mutex`                      | Multiple readers, exclusive writers   |
| AOF stream  | `std::mutex`                             | Single writer at a time               |
| Client jobs | `ThreadPool` + `std::condition_variable` | Workers sleep until job arrives       |
| Snapshot    | `SnapshotScheduler` thread               | Sleeps on interval, wakes on shutdown |

---

## Getting Started

### Prerequisites

- C++17 compiler
- CMake 3.15+
- Docker (optional)
- [nmap/ncat](https://nmap.org/download.html) for testing

For local builds:

- Windows: MSVC / MinGW with Winsock2
- Linux: GCC / Clang with POSIX sockets

---

## Build Locally

```bash
cmake -S . -B build
cmake --build build
```

Run:

```bash
./build/KV_Database
```

The server starts on port `6625` by default.

Type:

```
stop
```

in the terminal to shut it down cleanly.

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

The server will now be available on:

```
127.0.0.1:6625
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
│   └── Tests/
│       ├── test_hashmap.cpp
│       └── test_threadpool.cpp
├── docker/
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
- [x] Docker containerization
- [ ] Rate limiting (DDoS protection)
- [ ] Authentication (username + password)

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

> Built from scratch as a self-learning project to understand C++, networking, persistence, and systems programming.
