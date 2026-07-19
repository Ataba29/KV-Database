"""
Simple concurrent stress tester for kv-db.

Opens N concurrent TCP connections, has each one send a burst of
INSERT/GET/DELETE commands against unique keys, and reports connection
success rate, request throughput, and latency stats.

Usage:
    python stress_test.py --host 127.0.0.1 --port 6625 --clients 50 --requests 20

To disable rate limiting for an unthrottled baseline run, set the env var
on the server (no code changes / rebuild needed):
    docker run -d -p 6625:6625 -e KV_DISABLE_RATE_LIMIT=1 --name kv-stress kv-db:latest

Requires: Python 3.8+ (asyncio, no external dependencies)
"""

import asyncio
import argparse
import time
import statistics


async def run_client(
    client_id: int,
    host: str,
    port: int,
    num_requests: int,
    latencies: list,
    errors: list,
):
    try:
        reader, writer = await asyncio.open_connection(host, port)
    except Exception as e:
        errors.append(f"client {client_id}: connect failed: {e}")
        return

    inserted_keys = []  # keys this client has actually created

    try:
        for i in range(num_requests):
            # unique key per insert (client_id + i) so different clients and
            # different requests never collide on the same key/bucket
            if i % 3 == 0 or not inserted_keys:
                key = f"stress_key_{client_id}_{i}"
                cmd = f"INSERT {key} value_{i}\n"
                inserted_keys.append(key)
            elif i % 3 == 1:
                key = inserted_keys[-1]  # read a key that really exists
                cmd = f"GET {key}\n"
            else:
                key = inserted_keys.pop()  # delete a key that really exists
                cmd = f"DELETE {key}\n"

            start = time.perf_counter()
            writer.write(cmd.encode())
            await writer.drain()

            try:
                # adjust timeout if your server responses are naturally slower
                response = await asyncio.wait_for(reader.readline(), timeout=5.0)
                elapsed = time.perf_counter() - start
                latencies.append(elapsed)
                if not response:
                    errors.append(f"client {client_id}: connection closed by server")
                    break
            except asyncio.TimeoutError:
                errors.append(f"client {client_id}: timed out waiting for response")
                break
    except Exception as e:
        errors.append(f"client {client_id}: {e}")
    finally:
        writer.close()
        try:
            await writer.wait_closed()
        except Exception:
            pass


async def main():
    parser = argparse.ArgumentParser(description="Stress test kv-db")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=6625)
    parser.add_argument(
        "--clients", type=int, default=50, help="number of concurrent simulated clients"
    )
    parser.add_argument(
        "--requests", type=int, default=20, help="requests each client sends"
    )
    args = parser.parse_args()

    latencies = []
    errors = []

    print(
        f"Starting stress test: {args.clients} concurrent clients, "
        f"{args.requests} requests each, target {args.host}:{args.port}"
    )

    start = time.perf_counter()

    tasks = [
        run_client(i, args.host, args.port, args.requests, latencies, errors)
        for i in range(args.clients)
    ]
    await asyncio.gather(*tasks)

    total_time = time.perf_counter() - start
    total_requests = len(latencies)
    successful_clients = args.clients - len(
        {e.split(":")[0] for e in errors if "connect failed" in e}
    )

    print("\n--- Results ---")
    print(f"Total wall time:        {total_time:.2f}s")
    print(f"Clients attempted:      {args.clients}")
    print(f"Clients connected:      {successful_clients}")
    print(f"Total requests sent:    {total_requests}")
    print(f"Throughput:             {total_requests / total_time:.1f} req/sec")
    if latencies:
        print(f"Avg latency:            {statistics.mean(latencies)*1000:.2f} ms")
        print(f"p50 latency:            {statistics.median(latencies)*1000:.2f} ms")
        sorted_lat = sorted(latencies)
        p95_idx = int(len(sorted_lat) * 0.95)
        print(f"p95 latency:            {sorted_lat[p95_idx]*1000:.2f} ms")
        print(f"Max latency:            {max(latencies)*1000:.2f} ms")
    if errors:
        print(f"\nErrors: {len(errors)} (showing first 10)")
        for e in errors[:10]:
            print(f"  - {e}")


if __name__ == "__main__":
    asyncio.run(main())
