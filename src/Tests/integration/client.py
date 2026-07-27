import socket
import time


class ByteForgeClient:
    def __init__(self, host="127.0.0.1", port=6625, timeout=5.0,
                 max_retries=5, retry_delay=0.25):
        self.host = host
        self.port = port
        self.timeout = timeout
        self.max_retries = max_retries
        self.retry_delay = retry_delay
        self.sock = self._connect_with_retry()

    def _connect_with_retry(self):
        last_error = None
        for _ in range(self.max_retries):
            try:
                return socket.create_connection((self.host, self.port), timeout=self.timeout)
            except (ConnectionRefusedError, ConnectionResetError,
                    ConnectionAbortedError, OSError) as e:
                last_error = e
                time.sleep(self.retry_delay)
        raise ConnectionError(
            f"Failed to connect to {self.host}:{self.port} after {self.max_retries} attempts"
        ) from last_error

    def _send(self, command: str) -> str:
        last_error = None
        for _ in range(self.max_retries):
            try:
                self.sock.sendall((command + "\n").encode())
                return self._recv_line()
            except (ConnectionResetError, ConnectionAbortedError,
                    BrokenPipeError, OSError) as e:
                last_error = e
                try:
                    self.sock.close()
                except OSError:
                    pass
                time.sleep(self.retry_delay)
                self.sock = self._connect_with_retry()
        raise ConnectionError(
            f"Failed to send command after {self.max_retries} attempts"
        ) from last_error

    def _recv_line(self) -> str:
        data = b""
        while not data.endswith(b"\n"):
            chunk = self.sock.recv(1024)
            if not chunk:
                break
            data += chunk
        return data.decode().strip()

    def insert(self, key: str, value: str) -> str:
        return self._send(f"INSERT {key} {value}")

    def get(self, key: str) -> str:
        return self._send(f"GET {key}")

    def delete(self, key: str) -> str:
        return self._send(f"DELETE {key}")

    def close(self):
        self.sock.close()