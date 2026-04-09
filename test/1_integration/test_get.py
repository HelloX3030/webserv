#!/usr/bin/env python3

from common import ROOT, WebservRunner, assert_true, http10_request_bytes, open_client, read_http_response, run_test


def test_get_basic() -> None:
    target_file = ROOT / "www/full/files/itest_get_basic.txt"
    target_file.write_text("hello from get fixture\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_basic.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(
            "hello" in res.body_text.lower(),
            f"Expected body to contain hello, got: {res.body_text!r}",
        )
        assert_true(
            res.headers.get("connection", "").lower() == "close",
            f"Expected Connection: close for HTTP/1.0 default, got: {res.headers.get('connection')}",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_large_file() -> None:
    target_file = ROOT / "www/full/files/itest_get_large.bin"
    large_body = b"x" * (5 * 1024 * 1024)
    target_file.write_bytes(large_body)

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_large.bin",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client(timeout=5.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(
            len(res.body) == len(large_body),
            f"Expected {len(large_body)} bytes, got {len(res.body)}",
        )
        assert_true(
            res.body == large_body,
            "Large file body mismatch",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_404_not_found() -> None:
    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/nonexistent_file_xyz.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code == 404,
            f"Expected 404 for nonexistent file, got {res.status_code}",
        )


def test_get_multiple_sequential() -> None:
    files = {
        "itest_get_seq_1.txt": b"content1\n",
        "itest_get_seq_2.txt": b"content2\n",
        "itest_get_seq_3.txt": b"content3\n",
    }

    target_files = {
        name: ROOT / f"www/full/files/{name}"
        for name in files.keys()
    }

    for name, content in files.items():
        target_files[name].write_bytes(content)

    with WebservRunner("config/valid/full.conf"):
        for name, expected_content in files.items():
            req = http10_request_bytes(
                method="GET",
                target=f"/files/{name}",
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code == 200,
                f"Expected 200 for {name}, got {res.status_code}",
            )
            assert_true(
                res.body == expected_content,
                f"Content mismatch for {name}",
            )

    for target_file in target_files.values():
        if target_file.exists():
            target_file.unlink()


def test_get_keep_alive_multiple_requests() -> None:
    files = {
        "itest_get_ka_1.txt": b"keepalive1\n",
        "itest_get_ka_2.txt": b"keepalive2\n",
    }

    target_files = {
        name: ROOT / f"www/full/files/{name}"
        for name in files.keys()
    }

    for name, content in files.items():
        target_files[name].write_bytes(content)

    with WebservRunner("config/valid/full.conf"):
        with open_client(timeout=3.0) as sock:
            for name, expected_content in files.items():
                req = http10_request_bytes(
                    method="GET",
                    target=f"/files/{name}",
                    host="localhost",
                    headers={"Connection": "keep-alive"},
                    body=b"",
                )

                sock.sendall(req)
                res = read_http_response(sock)

                assert_true(
                    res.status_code == 200,
                    f"Expected 200 for {name}, got {res.status_code}",
                )
                assert_true(
                    res.body == expected_content,
                    f"Content mismatch for {name}",
                )
                assert_true(
                    res.headers.get("connection", "").lower() == "keep-alive",
                    f"Expected keep-alive for {name}",
                )

    for target_file in target_files.values():
        if target_file.exists():
            target_file.unlink()


def test_get_binary_file() -> None:
    target_file = ROOT / "www/full/files/itest_get_binary.bin"
    binary_content = bytes(range(256)) * 100
    target_file.write_bytes(binary_content)

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_binary.bin",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(
            res.body == binary_content,
            "Binary file body mismatch",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_custom_headers() -> None:
    target_file = ROOT / "www/full/files/itest_get_headers.txt"
    target_file.write_text("headers-test-content\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_headers.txt",
            host="localhost",
            headers={
                "User-Agent": "test-agent/1.0",
                "Accept": "text/plain",
                "Accept-Language": "en-US",
            },
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(
            "headers-test-content" in res.body_text,
            f"Unexpected response body: {res.body_text!r}",
        )
        assert_true(
            "content-length" in res.headers,
            "Missing Content-Length header",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_content_type_json() -> None:
    target_file = ROOT / "www/full/files/itest_get_data.json"
    json_content = '{"key": "value", "number": 42}\n'
    target_file.write_text(json_content, encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_data.json",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(
            res.body_text == json_content,
            f"JSON content mismatch",
        )
        assert_true(
            "content-type" in res.headers,
            "Missing Content-Type header",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_index_html() -> None:
    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(
            "webserv" in res.body_text.lower(),
            f"Expected index.html content, got: {res.body_text[:100]!r}",
        )


def test_get_response_headers_complete() -> None:
    target_file = ROOT / "www/full/files/itest_get_response.txt"
    target_file.write_text("response-check\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_response.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(
            "content-length" in res.headers,
            "Missing Content-Length",
        )
        assert_true(
            "connection" in res.headers,
            "Missing Connection header",
        )
        content_length = int(res.headers["content-length"])
        assert_true(
            len(res.body) == content_length,
            f"Content-Length mismatch: header={content_length}, actual={len(res.body)}",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_path_traversal_dotdot() -> None:
    with WebservRunner("config/valid/full.conf"):
        dangerous_paths = [
            "/files/../index.html",
            "/files/../../Makefile",
            "/files/%2e%2e%2findex.html",
        ]

        for path in dangerous_paths:
            req = http10_request_bytes(
                method="GET",
                target=path,
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (403, 404),
                f"Path traversal {path} should return 403 or 404, got {res.status_code}",
            )


def test_get_path_traversal_absolute() -> None:
    with WebservRunner("config/valid/full.conf"):
        dangerous_paths = [
            "/files//etc/passwd",
            "/files/etc/passwd",
            "/files/..%2fMakefile",
        ]

        for path in dangerous_paths:
            req = http10_request_bytes(
                method="GET",
                target=path,
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (403, 404),
                f"Path {path} should return 403 or 404, got {res.status_code}",
            )


def test_get_null_byte_injection() -> None:
    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/test%00.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (400, 403, 404),
            f"Null byte injection should fail, got {res.status_code}",
        )


def test_get_double_encoding() -> None:
    with WebservRunner("config/valid/full.conf"):
        double_encoded_paths = [
            "/files/%252e%252e%252findex.html",
            "/files/%25252e%25252e%252findex.html",
        ]

        for path in double_encoded_paths:
            req = http10_request_bytes(
                method="GET",
                target=path,
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (400, 403, 404),
                f"Double-encoded path {path} should fail, got {res.status_code}",
            )


def test_get_path_traversal_backslash() -> None:
    with WebservRunner("config/valid/full.conf"):
        paths = [
            "/files/..\\index.html",
            "/files/..\\..\\Makefile",
        ]

        for path in paths:
            req = http10_request_bytes(
                method="GET",
                target=path,
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (403, 404, 400),
                f"Backslash path {path} should fail, got {res.status_code}",
            )


def test_get_excessive_slashes() -> None:
    target_file = ROOT / "www/full/files/itest_get_slashes.txt"
    target_file.write_text("slash-test\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        paths = [
            "/files/itest_get_slashes.txt",
            "/files///itest_get_slashes.txt",
            "/files/itest_get_slashes.txt///",
            "//files//itest_get_slashes.txt",
        ]

        for path in paths:
            req = http10_request_bytes(
                method="GET",
                target=path,
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (200, 403, 404),
                f"Excessive slashes {path} should succeed or fail gracefully, got {res.status_code}",
            )

    if target_file.exists():
        target_file.unlink()


def test_get_hidden_files() -> None:
    hidden_file = ROOT / "www/full/files/.hidden"
    hidden_file.write_text("secret\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/.hidden",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code == 200,
            f"Hidden files should be accessible (server has no filtering), got {res.status_code}",
        )

    if hidden_file.exists():
        hidden_file.unlink()


def test_get_special_chars_filename() -> None:
    special_file = ROOT / "www/full/files/itest_get_special_%20%23.txt"
    safe_filename = "itest_get_special_____.txt"
    safe_path = ROOT / f"www/full/files/{safe_filename}"
    safe_path.write_text("special-chars\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target=f"/files/{safe_filename}",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code == 200,
            f"Special chars filename should work, got {res.status_code}",
        )

    if safe_path.exists():
        safe_path.unlink()


def test_get_symlink_escape() -> None:
    import os

    symlink_path = ROOT / "www/full/files/itest_get_symlink_escape"
    target_path = ROOT / "Makefile"

    try:
        if symlink_path.exists():
            symlink_path.unlink()
        os.symlink(target_path, symlink_path)
    except (OSError, NotImplementedError):
        return

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_symlink_escape",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client(timeout=3.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (200, 403),
            f"Symlink should either be followed safely or rejected, got {res.status_code}",
        )

    if symlink_path.exists():
        symlink_path.unlink()


def main() -> int:
    tests = [
        ("GET basic file", test_get_basic),
        ("GET large 5MB file", test_get_large_file),
        ("GET 404 not found", test_get_404_not_found),
        ("GET multiple sequential requests", test_get_multiple_sequential),
        ("GET keep-alive multiple requests", test_get_keep_alive_multiple_requests),
        ("GET binary file", test_get_binary_file),
        ("GET with custom headers", test_get_custom_headers),
        ("GET JSON Content-Type", test_get_content_type_json),
        ("GET index.html", test_get_index_html),
        ("GET response headers complete", test_get_response_headers_complete),
        ("GET path traversal dotdot", test_get_path_traversal_dotdot),
        ("GET path traversal absolute", test_get_path_traversal_absolute),
        ("GET null byte injection", test_get_null_byte_injection),
        ("GET double encoding", test_get_double_encoding),
        ("GET path traversal backslash", test_get_path_traversal_backslash),
        ("GET excessive slashes", test_get_excessive_slashes),
        ("GET hidden files", test_get_hidden_files),
        ("GET special chars filename", test_get_special_chars_filename),
        ("GET symlink escape", test_get_symlink_escape),
    ]

    passed = 0
    failed = 0

    for name, test_fn in tests:
        if run_test(name, test_fn) == 0:
            passed += 1
        else:
            failed += 1

    if failed > 0:
        print(f"\n{failed} test(s) failed, {passed} passed")
        return 1

    print(f"\nAll {passed} GET tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
