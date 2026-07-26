# Integration Tests

Integration tests for ByteForge, run against a real, running server instance
over TCP. These are not unit tests — they require ByteForge to actually be
running on `127.0.0.1:6625` before you start pytest.

## Prerequisites

- Python 3.11+
- A running ByteForge server (either built locally or via Docker)

## 1. Set up a virtual environment

Create the `.venv` at the **repo root** (same level as `CMakeLists.txt`), not
inside `Tests/`.

**Linux / macOS:**
```bash
python3 -m venv .venv
source .venv/bin/activate
```

**Windows (PowerShell):**
```powershell
python -m venv .venv
.venv\Scripts\Activate.ps1
```

You should see `(.venv)` appear in your prompt once it's active.

## 2. Install test dependencies

From the repo root, with the venv activated:

```bash
pip install -r src/Tests/integration/requirements.txt
```

This installs `pytest` and anything else the tests need. The client itself
(`client.py`) only uses the Python standard library (`socket`), so there's
nothing else to install for it.

## 3. Start the server

Pick one:

**Option A — build and run locally:**
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ByteForge   # adjust to your actual binary name/path
```

**Option B — run via Docker:**
```bash
docker build -t byteforge:local .
docker run -d --name byteforge -p 6625:6625 byteforge:local
```

Either way, confirm the server printed something like:
`[SERVER] Server is now accepting connections!`

before running tests — the tests don't wait for the server to be ready
themselves, and will fail with `ConnectionRefusedError` if it's not up yet.

## 4. Run the tests

From the repo root, with the venv still activated:

```bash
pytest src/Tests/integration -v
```

Run a single file:
```bash
pytest src/Tests/integration/test_basic_commands.py -v
```

Run a single test:
```bash
pytest src/Tests/integration/test_advanced.py::test_unknown_command -v
```

## 5. Stop the server

If you ran it via Docker:
```bash
docker rm -f byteforge
```

If you ran it locally, type `stop` in the server's console (per `main.cpp`'s
input loop), or Ctrl+C.

## Test files

- **`client.py`** — a small TCP client (`ByteForgeClient`) that speaks
  ByteForge's newline-delimited protocol (`INSERT key value\n`, `GET key\n`,
  `DELETE key\n`). Includes retry/reconnect logic for transient connection
  drops (e.g. rate-limit resets), so other test files can just use it
  directly without worrying about flakiness at the socket level.
- **`test_basic_commands.py`** — INSERT/GET/DELETE roundtrip, and confirms a
  deleted key reads back as not found.
- **`test_advanced.py`** — unknown command handling, concurrent clients on
  distinct keys, cross-client visibility of a delete, and rate limiter
  behavior (burst capacity + refill).

## Notes

- ByteForge's rate limiter has a **burst capacity of 10** connections per IP,
  refilling at **5/sec**. If you run tests repeatedly in quick succession
  (e.g. re-running the whole suite back-to-back), you may see transient
  connection resets while the bucket refills. `ByteForgeClient` retries
  through these automatically; if you write new tests with raw sockets
  instead of the client, keep this in mind.
- The server persists state (AOF/RDB), so if you're adding new tests, give
  them their own unique keys rather than reusing ones from other tests, to
  avoid cross-test interference.