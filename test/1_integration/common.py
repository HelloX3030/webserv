#!/usr/bin/env python3

from __future__ import annotations

import socket
import subprocess
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Optional

ROOT = Path(__file__).resolve().parents[2]
BINARY = ROOT / "webserv"
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
        ensure_binary()
        if not self.config_path.exists():
            raise RuntimeError(f"Config not found: {self.config_path}")

        self.process = subprocess.Popen(
            [str(BINARY), str(self.config_path)],
            cwd=str(ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )

        deadline = time.time() + 5.0
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
    sock = socket.create_connection((HOST, PORT), timeout=timeout)
    sock.settimeout(timeout)
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


def run_test(name: str, fn) -> int:
    print(f"[RUN] {name}")
    try:
        fn()
    except Exception as exc:
        print(f"[FAIL] {name}: {exc}")
        return 1
    print(f"[PASS] {name}")
    return 0
