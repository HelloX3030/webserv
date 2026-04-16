#!/usr/bin/env python3
import socket
import threading
import time

HOST = "127.0.0.1"
PORT = 8080
PATH = "/directory/youpi.bla"
WORKERS = 20
ITERATIONS = 5
BODY_SIZE = 100_000_000

failures = []


def worker(worker_id):
    body = b"X" * BODY_SIZE
    request = (
        f"POST {PATH} HTTP/1.1\r\n".encode()
        + b"Host: localhost\r\n"
        + b"Connection: close\r\n"
        + f"Content-Length: {len(body)}\r\n\r\n".encode()
        + body
    )

    for iteration in range(ITERATIONS):
        try:
            sock = socket.create_connection((HOST, PORT), timeout=30)
            sock.settimeout(30)
            sock.sendall(request)
            while sock.recv(4096):
                pass
            sock.close()
        except Exception as exc:
            failures.append((worker_id, iteration, type(exc).__name__, str(exc)))
            return


threads = []
for worker_id in range(WORKERS):
    thread = threading.Thread(target=worker, args=(worker_id,))
    threads.append(thread)
    thread.start()
    time.sleep(0.01)

for thread in threads:
    thread.join()

if failures:
    print("FAILED")
    for worker_id, iteration, exc_name, message in failures[:10]:
        print(f"worker={worker_id} iteration={iteration} error={exc_name}: {message}")
    raise SystemExit(1)

print("OK")
