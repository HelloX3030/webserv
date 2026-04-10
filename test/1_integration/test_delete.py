#!/usr/bin/env python3

import socket

from common import ROOT, WebservRunner, TestRunner, assert_true, http10_request_bytes, open_client, read_http_response


def test_delete_basic() -> None:
    target_file = ROOT / "www/full/files/itest_delete_basic.txt"
    target_file.write_text("delete-me\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_basic.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(not target_file.exists(), f"Expected file to be deleted: {target_file}")

    if target_file.exists():
        target_file.unlink()


def test_delete_missing_file() -> None:
    target_file = ROOT / "www/full/files/itest_delete_missing.txt"
    if target_file.exists():
        target_file.unlink()

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_missing.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 404, f"Expected 404 for missing file, got {res.status_code}")


def test_delete_directory() -> None:
    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 403, f"Expected 403 for directory delete, got {res.status_code}")


def test_delete_path_traversal() -> None:
    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/../Makefile",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 403, f"Expected 403 for traversal delete, got {res.status_code}")


def test_delete_query_string() -> None:
    target_file = ROOT / "www/full/files/itest_delete_query.txt"
    target_file.write_text("delete-with-query\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_query.txt?foo=bar&baz=qux",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200 for delete with query, got {res.status_code}")
        assert_true(not target_file.exists(), "File should be deleted even with query string")

    if target_file.exists():
        target_file.unlink()


def test_delete_special_filename() -> None:
    target_file = ROOT / "www/full/files/itest_delete_special_name.txt"
    target_file.write_text("special delete\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_special_name.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200 for special-name delete, got {res.status_code}")
        assert_true(not target_file.exists(), "Special-name file should be deleted")

    if target_file.exists():
        target_file.unlink()


def test_delete_sequential() -> None:
    target_file = ROOT / "www/full/files/itest_delete_sequential.txt"
    target_file.write_text("seq delete\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req1 = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_sequential.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req1)
            res1 = read_http_response(sock)

        assert_true(res1.status_code == 200, f"First delete should be 200, got {res1.status_code}")

        req2 = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_sequential.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req2)
            res2 = read_http_response(sock)

        assert_true(res2.status_code == 404, f"Second delete should be 404, got {res2.status_code}")

    if target_file.exists():
        target_file.unlink()


def test_delete_keep_alive_multiple() -> None:
    files = []
    for index in range(3):
        target_file = ROOT / f"www/full/files/itest_delete_ka_{index}.txt"
        target_file.write_text(f"delete {index}\n", encoding="utf-8")
        files.append(target_file)

    with WebservRunner("config/valid/full.conf"):
        with open_client() as sock:
            for index in range(3):
                req = http10_request_bytes(
                    method="DELETE",
                    target=f"/files/itest_delete_ka_{index}.txt",
                    host="localhost",
                    headers={"Connection": "keep-alive"},
                    body=b"",
                )

                sock.sendall(req)
                res = read_http_response(sock)
                assert_true(res.status_code == 200, f"Keep-alive delete {index} got {res.status_code}")

    for target_file in files:
        if target_file.exists():
            target_file.unlink()


def test_delete_invalid_http_version() -> None:
    target_file = ROOT / "www/full/files/itest_delete_http_version.txt"
    target_file.write_text("version check\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 8080))

        request = b"DELETE /files/itest_delete_http_version.txt HTTP/2.5\r\nHost: localhost\r\n\r\n"
        try:
            sock.sendall(request)
            sock.settimeout(2.0)
            response = sock.recv(4096)
            assert_true(len(response) > 0, "Invalid HTTP version should return an error response")
        finally:
            sock.close()

    if target_file.exists():
        target_file.unlink()


def test_delete_very_long_header() -> None:
    target_file = ROOT / "www/full/files/itest_delete_longhdr.txt"
    target_file.write_text("header test\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_longhdr.txt",
            host="localhost",
            headers={"X-Custom-Long": "x" * 8192},
            body=b"",
        )

        with open_client(timeout=3.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200 for long-header delete, got {res.status_code}")
        assert_true(not target_file.exists(), "File should be deleted even with long header")

    if target_file.exists():
        target_file.unlink()


def main() -> int:
    tests = [
        ("DELETE basic", test_delete_basic),
        ("DELETE missing file", test_delete_missing_file),
        ("DELETE directory", test_delete_directory),
        ("DELETE path traversal", test_delete_path_traversal),
        ("DELETE query string", test_delete_query_string),
        ("DELETE special filename", test_delete_special_filename),
        ("DELETE sequential", test_delete_sequential),
        ("DELETE keep-alive multiple", test_delete_keep_alive_multiple),
        ("DELETE invalid HTTP version", test_delete_invalid_http_version),
        ("DELETE very long header", test_delete_very_long_header),
    ]

    runner = TestRunner("DELETE", len(tests))
    for name, test_fn in tests:
        runner.run(name, test_fn)

    return runner.summary()


if __name__ == "__main__":
    raise SystemExit(main())
