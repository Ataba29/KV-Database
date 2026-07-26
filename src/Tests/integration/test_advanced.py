import socket
import threading

import pytest
from client import ByteForgeClient


@pytest.fixture
def client():
    c = ByteForgeClient()
    yield c
    c.close()


def test_unknown_command(client):
    response = client._send("FOOBAR volt 3333")
    assert response == "No command was received"


def test_concurrent_clients_distinct_keys():
    # Kept comfortably under the rate limiter's burst capacity (10),
    # since earlier tests in the session may have already spent tokens
    # that haven't fully refilled yet.
    num_clients = 5
    results: list[str | None] = [None] * num_clients
    errors: list[str | None] = [None] * num_clients

    def worker(i):
        try:
            c = ByteForgeClient()
            key = f"vault{i}"
            value = str(1000 + i)
            c.insert(key, value)
            results[i] = c.get(key)
            c.delete(key)
            c.close()
        except OSError as e:
            errors[i] = str(e)

    threads = [threading.Thread(target=worker, args=(i,)) for i in range(num_clients)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    for i in range(num_clients):
        assert errors[i] is None, f"client {i} failed: {errors[i]}"
        expected_value = str(1000 + i)
        assert results[i] == f"Get command was successful: {expected_value}"


def test_cross_client_visibility_after_delete():
    client_a = ByteForgeClient()
    client_b = ByteForgeClient()

    try:
        assert client_a.insert("shared", "42") == "Insert command was successful"
        assert client_a.delete("shared") == "Delete command was successful"

        response = client_b.get("shared")
        assert response == "Get command was successful but key dont exist"
    finally:
        client_a.close()
        client_b.close()


def test_rate_limiter_blocks_excess_connections():
    host, port = "127.0.0.1", 6625
    max_attempts = 50
    blocked = False

    for _ in range(max_attempts):
        try:
            s = socket.create_connection((host, port), timeout=1.0)
            s.sendall(b"GET volt\n")
            data = s.recv(1024)
            s.close()

            if data == b"":
                blocked = True
                break
        except (ConnectionResetError, ConnectionAbortedError, BrokenPipeError, OSError):
            blocked = True
            break

    assert blocked, f"Expected at least one blocked connection within {max_attempts} attempts"