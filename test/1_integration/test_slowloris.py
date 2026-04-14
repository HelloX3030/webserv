#!/usr/bin/env python3

import socket
import time

from common import ROOT, USE_VALGRIND, WebservRunner, TestRunner, assert_true, open_client


def _is_closed_after_probe(sock: socket.socket) -> bool:
    try:
        sock.sendall(b"x")
        data = sock.recv(1)
        return len(data) == 0
    except (BrokenPipeError, ConnectionResetError):
        return True
    except socket.timeout:
        return False


def test_slowloris_single_connection_timeout() -> None:
    if USE_VALGRIND:
        return

    with WebservRunner("config/valid/full.conf"):
        with open_client(timeout=1.0) as sock:
            # Send an incomplete header block then stall longer than server idle timeout.
            sock.sendall(
                b"GET / HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"X-Slow: "
            )

            time.sleep(3.0)

            closed = _is_closed_after_probe(sock)
            assert_true(closed, "Server should close stalled slowloris connection")


def test_slowloris_multiple_stalled_connections_timeout() -> None:
    if USE_VALGRIND:
        return

    sockets = []

    try:
        with WebservRunner("config/valid/full.conf"):
            for i in range(12):
                sock = open_client(timeout=1.0)
                sock.sendall(
                    b"GET / HTTP/1.1\r\n"
                    b"Host: localhost\r\n"
                    + f"X-Slow-{i}: ".encode("ascii")
                )
                sockets.append(sock)

            time.sleep(3.0)

            closed_count = 0
            for sock in sockets:
                if _is_closed_after_probe(sock):
                    closed_count += 1

            assert_true(
                closed_count == len(sockets),
                f"Expected all stalled connections closed, got {closed_count}/{len(sockets)}",
            )
    finally:
        for sock in sockets:
            try:
                sock.close()
            except OSError:
                pass


def test_slowloris_stalled_body_timeout() -> None:
    if USE_VALGRIND:
        return

    target_file = ROOT / "www/full/files/itest_slowloris_body_timeout.txt"
    if target_file.exists():
        target_file.unlink()

    with WebservRunner("config/valid/full.conf"):
        with open_client(timeout=1.0) as sock:
            sock.sendall(
                b"POST /files/itest_slowloris_body_timeout.txt HTTP/1.1\r\n"
                b"Host: localhost\r\n"
                b"Content-Type: text/plain\r\n"
                b"Content-Length: 12\r\n"
                b"\r\n"
                b"ab"
            )

            time.sleep(3.0)

            closed = _is_closed_after_probe(sock)
            assert_true(closed, "Server should close stalled slow body upload")

    assert_true(not target_file.exists(), "Partial timed-out upload should not create file")


def main() -> int:
    tests = [
        ("Slowloris single stalled connection", test_slowloris_single_connection_timeout),
        ("Slowloris multiple stalled connections", test_slowloris_multiple_stalled_connections_timeout),
        ("Slowloris stalled request body", test_slowloris_stalled_body_timeout),
    ]

    runner = TestRunner("Slowloris", len(tests))
    for name, test_fn in tests:
        runner.run(name, test_fn)

    return runner.summary()


if __name__ == "__main__":
    raise SystemExit(main())
