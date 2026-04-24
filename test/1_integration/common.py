#!/usr/bin/env python3

from __future__ import annotations

import os
import socket
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple

ROOT = Path(__file__).resolve().parents[2]

# Check environment variable for custom binary path, otherwise use default
_default_binary = ROOT / "webserv"
BINARY = Path(os.environ.get("WEBSERV_BINARY", str(_default_binary)))
USE_VALGRIND = os.environ.get("WEBSERV_VALGRIND", "0") == "1"

# Multiply timeouts when running with valgrind (adds significant overhead)
# Valgrind can slow things down 10-30x depending on the operation
TIMEOUT_MULTIPLIER = 50.0 if USE_VALGRIND else 1.0

HOST = "127.0.0.1"
PORT = 8080


@dataclass
class HttpResponse:
    status_line: str
    status_code: int
    reason: str
    headers: Dict[str, str]
    body: bytes

    @property
    def body_text(self) -> str:
        return self.body.decode("utf-8", errors="replace")


class WebservRunner:
    def __init__(self, config_rel_path: str):
        self.config_path = ROOT / config_rel_path
        self.process: Optional[subprocess.Popen] = None

    def __enter__(self) -> "WebservRunner":
        self.start()
        return self

    def __exit__(self, exc_type, exc, tb) -> None:
        self.stop()

    def start(self) -> None:
        # Fail fast if some other process is already listening.
        # Otherwise tests may accidentally talk to the wrong server.
        try:
            with socket.create_connection((HOST, PORT), timeout=0.2):
                raise RuntimeError(
                    f"Port {HOST}:{PORT} is already in use. "
                    "Stop the running server before executing integration tests."
                )
        except OSError:
            pass

        ensure_binary()
        if not self.config_path.exists():
            raise RuntimeError(f"Config not found: {self.config_path}")

        # Build command with optional valgrind wrapper
        cmd: List[str] = []
        if USE_VALGRIND:
            cmd.extend([
                "valgrind",
                "--leak-check=full",
                "--track-fds=yes",
                "--show-leak-kinds=all",
                "--error-exitcode=1",
                "--log-file=/tmp/valgrind-%p.log",
            ])
        cmd.extend([str(BINARY), str(self.config_path)])

        self.process = subprocess.Popen(
            cmd,
            cwd=str(ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        # Increase startup deadline when running with valgrind
        startup_timeout = 5.0 * TIMEOUT_MULTIPLIER
        deadline = time.time() + startup_timeout
        while time.time() < deadline:
            if self.process.poll() is not None:
                out, err = self.process.communicate(timeout=1)
                raise RuntimeError(
                    "webserv exited during startup\n"
                    f"config: {self.config_path}\n"
                    f"stdout:\n{out}\n"
                    f"stderr:\n{err}"
                )

            try:
                with socket.create_connection((HOST, PORT), timeout=0.2):
                    return
            except OSError:
                time.sleep(0.1)

        self.stop()
        raise RuntimeError(
            f"webserv did not start listening on {HOST}:{PORT} within timeout"
        )

    def stop(self) -> None:
        if self.process is None:
            return

        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=2)

        self.process = None


def ensure_binary() -> None:
    if BINARY.exists():
        return

    # If custom binary is specified and doesn't exist, that's an error
    if os.environ.get("WEBSERV_BINARY"):
        raise RuntimeError(
            f"Custom binary specified but not found: {BINARY}"
        )

    # Otherwise, build the default binary
    result = subprocess.run(
        ["make", "all"],
        cwd=str(ROOT),
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        raise RuntimeError(
            "Failed to build webserv binary\n"
            f"stdout:\n{result.stdout}\n"
            f"stderr:\n{result.stderr}"
        )


def assert_true(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def http10_request_bytes(method: str, target: str, host: str, headers: Dict[str, str], body: bytes) -> bytes:
    lines = [f"{method} {target} HTTP/1.0", f"Host: {host}"]

    for key, value in headers.items():
        lines.append(f"{key}: {value}")

    lines.append("")
    lines.append("")
    head = "\r\n".join(lines).encode("utf-8")
    return head + body


def open_client(timeout: float = 2.0) -> socket.socket:
    # Apply timeout multiplier when running with valgrind (5x slower)
    adjusted_timeout = timeout * TIMEOUT_MULTIPLIER
    sock = socket.create_connection((HOST, PORT), timeout=adjusted_timeout)
    sock.settimeout(adjusted_timeout)
    return sock


def read_http_response(sock: socket.socket) -> HttpResponse:
    data = b""
    while b"\r\n\r\n" not in data:
        chunk = sock.recv(4096)
        if not chunk:
            break
        data += chunk

    if b"\r\n\r\n" not in data:
        raise AssertionError("Did not receive full HTTP header block")

    head, body = data.split(b"\r\n\r\n", 1)
    lines = head.decode("iso-8859-1").split("\r\n")

    if not lines or not lines[0].startswith("HTTP/"):
        raise AssertionError(f"Invalid status line: {lines[0] if lines else '<empty>'}")

    status_parts = lines[0].split(" ", 2)
    if len(status_parts) < 2:
        raise AssertionError(f"Malformed status line: {lines[0]}")

    status_code = int(status_parts[1])
    reason = status_parts[2] if len(status_parts) > 2 else ""

    headers: Dict[str, str] = {}
    for line in lines[1:]:
        if not line:
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        headers[key.strip().lower()] = value.strip()

    content_length = 0
    if "content-length" in headers:
        content_length = int(headers["content-length"])

    while len(body) < content_length:
        chunk = sock.recv(4096)
        if not chunk:
            break
        body += chunk

    if len(body) < content_length:
        raise AssertionError(
            f"Body too short: expected {content_length} bytes, got {len(body)}"
        )

    return HttpResponse(
        status_line=lines[0],
        status_code=status_code,
        reason=reason,
        headers=headers,
        body=body[:content_length],
    )


def read_http_response_buffered(sock: socket.socket, buffer: bytearray) -> HttpResponse:
    while b"\r\n\r\n" not in buffer:
        chunk = sock.recv(4096)
        if not chunk:
            break
        buffer.extend(chunk)

    if b"\r\n\r\n" not in buffer:
        raise AssertionError("Did not receive full HTTP header block")

    head, remainder = bytes(buffer).split(b"\r\n\r\n", 1)
    lines = head.decode("iso-8859-1").split("\r\n")

    if not lines or not lines[0].startswith("HTTP/"):
        raise AssertionError(f"Invalid status line: {lines[0] if lines else '<empty>'}")

    status_parts = lines[0].split(" ", 2)
    if len(status_parts) < 2:
        raise AssertionError(f"Malformed status line: {lines[0]}")

    status_code = int(status_parts[1])
    reason = status_parts[2] if len(status_parts) > 2 else ""

    headers: Dict[str, str] = {}
    for line in lines[1:]:
        if not line:
            continue
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        headers[key.strip().lower()] = value.strip()

    content_length = 0
    if "content-length" in headers:
        content_length = int(headers["content-length"])

    body = bytearray(remainder)
    while len(body) < content_length:
        chunk = sock.recv(4096)
        if not chunk:
            break
        body.extend(chunk)

    if len(body) < content_length:
        raise AssertionError(
            f"Body too short: expected {content_length} bytes, got {len(body)}"
        )

    consumed = len(head) + 4 + content_length
    del buffer[:consumed]

    return HttpResponse(
        status_line=lines[0],
        status_code=status_code,
        reason=reason,
        headers=headers,
        body=bytes(body[:content_length]),
    )


def run_test(name: str, fn) -> Tuple[int, Optional[str]]:
    """
    Run a single test and return (status_code, error_message_or_none).
    Status: 0 = pass, 1 = fail
    """
    try:
        fn()
        return (0, None)
    except Exception as exc:
        return (1, f"{name}: {str(exc)}")


class TestRunner:
    """Helper for running tests with nice progress output."""

    def __init__(self, test_name: str, test_count: int):
        self.test_name = test_name
        self.test_count = test_count
        self.passed = 0
        self.failed = 0
        self.failures: list[str] = []
        print(f"test {test_name}")

    def run(self, test_name: str, test_fn) -> bool:
        """Run a test and update progress. Returns True if passed."""
        current = self.passed + self.failed + 1
        status, error = run_test(test_name, test_fn)

        if status == 0:
            self.passed += 1
            result = "✓"
        else:
            self.failed += 1
            self.failures.append(error)
            result = "✗"

        # Overwrite same line with progress
        print(f"\r  {result} {current:2d}/{self.test_count} tests", end="", flush=True)
        return status == 0

    def summary(self) -> int:
        """Print summary and return exit code."""
        print()  # Newline after progress

        if self.failed > 0:
            print(f"  ✗ Failed tests ({self.failed}):")
            for failure in self.failures:
                print(f"    • {failure}")
            print(f"  Summary: {self.failed} failed, {self.passed} passed\n")
            return 1

        print(f"  ✓ All {self.passed} {self.test_name} tests passed\n")
        return 0
