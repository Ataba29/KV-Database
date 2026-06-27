# kv-db

A lightweight, persistent key-value database built from scratch in C++, containerized with Docker. This project was built as a self-learning exercise to deeply understand systems programming, networking, and concurrency in C++.

---

## About

kv-db is a Redis-inspired key-value store that supports basic CRUD operations over a TCP connection. It is designed around a phonebook use case where string keys map to string values.

This is a personal learning project. The goal is not to build a production database, but to understand how databases, servers, and concurrency actually work under the hood.

---

## Features

- **Custom HashMap** — hand-rolled hash table with separate chaining for collision resolution and dynamic growth
- **TCP Server** — raw socket server built with Winsock2 that accepts and handles client connections
- **Command Parser** — parses `INSERT`, `GET`, and `DELETE` commands from raw TCP bytes
- **Persistence** — hybrid durability layer combining:
  - **AOF (Append-Only File)** — logs every write operation in real time
  - **RDB Snapshots** — full point-in-time dumps of the database every 5 minutes
- **Thread Pool** — _(in progress)_ manages concurrent client sessions efficiently
- **Docker** — _(in progress)_ containerized for consistent deployment

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
   TCP Server (Winsock2)
        │
        ├──▶ Command Parser
        │         │
        │         ▼
        │    Thread Pool (workers)
        │         │
        │         ▼
        │    HashMap (in-memory store)
        │
        └──▶ Persistence Layer
                  ├── appendonly.aof  (real-time log)
                  └── snapshot.rdb    (periodic snapshot)
```

---

## Getting Started

### Prerequisites

- Windows (Winsock2)
- CMake 3.15+
- A C++17 compiler (MSVC or MinGW)
- [nmap/ncat](https://nmap.org/download.html) for testing

### Build

```bash
cmake -S . -B build
cmake --build build
```

### Run

```bash
./build/KV_Database
```

The server starts on port `6625` by default.

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
│   └── Persistence/
│       ├── Persistence.h
│       └── Persistence.cpp
├── tests/
├── docker/
├── data/
├── CMakeLists.txt
└── README.md
```

---

## Roadmap

- [x] Custom HashMap with separate chaining
- [x] TCP server with Winsock2
- [x] Command parser (INSERT, GET, DELETE)
- [x] AOF persistence
- [x] RDB snapshots
- [ ] Thread pool for concurrent clients
- [ ] Connect persistence to live server
- [ ] Docker containerization
- [ ] Rate limiting
- [ ] Authentication (username + password)

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

---

> Built from scratch as a self-learning project to understand C++, networking, and systems programming.
