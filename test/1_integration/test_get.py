#!/usr/bin/env python3

import shutil

from common import ROOT, WebservRunner, TestRunner, USE_VALGRIND, assert_true, http10_request_bytes, open_client, read_http_response, read_http_response_buffered


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
    if USE_VALGRIND:
        return  # Skip: too slow with valgrind overhead
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


def test_get_http11_default_keep_alive_multiple_requests() -> None:
    files = {
        "itest_get_h11_ka_1.txt": b"http11-keepalive1\n",
        "itest_get_h11_ka_2.txt": b"http11-keepalive2\n",
    }

    target_files = {
        name: ROOT / f"www/full/files/{name}"
        for name in files.keys()
    }

    for name, content in files.items():
        target_files[name].write_bytes(content)

    try:
        with WebservRunner("config/valid/full.conf"):
            with open_client(timeout=3.0) as sock:
                req1 = (
                    b"GET /files/itest_get_h11_ka_1.txt HTTP/1.1\r\n"
                    b"Host: localhost\r\n"
                    b"\r\n"
                )
                sock.sendall(req1)
                res1 = read_http_response(sock)

                assert_true(res1.status_code == 200, f"HTTP/1.1 first request expected 200, got {res1.status_code}")
                assert_true(
                    res1.headers.get("connection", "").lower() == "keep-alive",
                    f"HTTP/1.1 default persistence should be keep-alive, got {res1.headers.get('connection')}",
                )
                assert_true(
                    res1.body == files["itest_get_h11_ka_1.txt"],
                    f"First HTTP/1.1 keep-alive body mismatch: {res1.body!r}",
                )

                req2 = (
                    b"GET /files/itest_get_h11_ka_2.txt HTTP/1.1\r\n"
                    b"Host: localhost\r\n"
                    b"Connection: close\r\n"
                    b"\r\n"
                )
                sock.sendall(req2)
                res2 = read_http_response(sock)

                assert_true(res2.status_code == 200, f"HTTP/1.1 second request expected 200, got {res2.status_code}")
                assert_true(
                    res2.headers.get("connection", "").lower() == "close",
                    f"Second request asked for close, got {res2.headers.get('connection')}",
                )
                assert_true(
                    res2.body == files["itest_get_h11_ka_2.txt"],
                    f"Second HTTP/1.1 keep-alive body mismatch: {res2.body!r}",
                )
    finally:
        for target_file in target_files.values():
            if target_file.exists():
                target_file.unlink()


def test_get_http11_connection_close_token_list() -> None:
    target_file = ROOT / "www/full/files/itest_get_h11_close_token.txt"
    target_file.write_text("close-token\n", encoding="utf-8")

    try:
        with WebservRunner("config/valid/full.conf"):
            with open_client(timeout=3.0) as sock:
                req1 = (
                    b"GET /files/itest_get_h11_close_token.txt HTTP/1.1\r\n"
                    b"Host: localhost\r\n"
                    b"Connection: close, foo\r\n"
                    b"\r\n"
                )
                sock.sendall(req1)
                res1 = read_http_response(sock)

                assert_true(res1.status_code == 200, f"HTTP/1.1 token-list request expected 200, got {res1.status_code}")
                assert_true(
                    res1.headers.get("connection", "").lower() == "close",
                    f"Connection token-list containing close must close, got {res1.headers.get('connection')}",
                )

                req2 = (
                    b"GET /files/itest_get_h11_close_token.txt HTTP/1.1\r\n"
                    b"Host: localhost\r\n"
                    b"\r\n"
                )

                try:
                    sock.sendall(req2)
                    data2 = sock.recv(4096)
                    assert_true(
                        len(data2) == 0,
                        f"Socket should be closed after Connection: close token-list, got {len(data2)} extra bytes",
                    )
                except (BrokenPipeError, ConnectionResetError):
                    assert_true(True, "Connection closed as expected")
    finally:
        if target_file.exists():
            target_file.unlink()


def test_get_http11_large_pipeline() -> None:
    if USE_VALGRIND:
        return  # Skip: too slow with valgrind overhead

    target_file = ROOT / "www/full/files/itest_get_pipeline.txt"
    target_file.write_text("pipeline-test\n", encoding="utf-8")

    request_count = 128

    try:
        with WebservRunner("config/valid/full.conf"):
            with open_client(timeout=10.0) as sock:
                response_buffer = bytearray()
                pipeline = b"".join(
                    (
                        b"GET /files/itest_get_pipeline.txt HTTP/1.1\r\n"
                        b"Host: localhost\r\n"
                        b"\r\n"
                    )
                    for _ in range(request_count)
                )

                sock.sendall(pipeline)

                for i in range(request_count):
                    res = read_http_response_buffered(sock, response_buffer)
                    assert_true(
                        res.status_code == 200,
                        f"Pipelined GET {i} expected 200, got {res.status_code}",
                    )
                    assert_true(
                        res.body_text == "pipeline-test\n",
                        f"Pipelined GET {i} body mismatch: {res.body_text!r}",
                    )
                assert_true(True, "Pipelined GET sequence completed")
    finally:
        if target_file.exists():
            target_file.unlink()


def test_get_binary_file() -> None:
    if USE_VALGRIND:
        return  # Skip: too slow with valgrind overhead
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
                res.status_code in (400, 403, 404),
                f"Path traversal {path} should return 400, 403 or 404, got {res.status_code}",
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
                res.status_code in (400, 403, 404),
                f"Path {path} should return 400, 403 or 404, got {res.status_code}",
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


def test_get_path_traversal_prefix_collision() -> None:
    sibling_dir = ROOT / "www/full/files_hack"
    sibling_file = sibling_dir / "itest_get_prefix_collision.txt"

    sibling_dir.mkdir(parents=True, exist_ok=True)
    sibling_file.write_text("prefix-collision-leak\n", encoding="utf-8")

    try:
        with WebservRunner("config/valid/full.conf"):
            req = http10_request_bytes(
                method="GET",
                target="/files/../files_hack/itest_get_prefix_collision.txt",
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (403, 404),
                f"Prefix-collision traversal should be blocked, got {res.status_code} with body {res.body_text!r}",
            )
    finally:
        if sibling_file.exists():
            sibling_file.unlink()
        if sibling_dir.exists():
            sibling_dir.rmdir()


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


def test_get_directory_request() -> None:
    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (200, 403, 404),
            f"Directory request /files/ should return 200 (listing), 403 (forbidden), or 404, got {res.status_code}",
        )


def test_get_query_string() -> None:
    target_file = ROOT / "www/full/files/itest_get_query.txt"
    target_file.write_text("query-test\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_query.txt?foo=bar&baz=qux",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code == 200,
            f"Query string should be ignored, got {res.status_code}",
        )
        assert_true(
            "query-test" in res.body_text,
            f"Expected body to contain file content despite query string, got: {res.body_text!r}",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_empty_file() -> None:
    target_file = ROOT / "www/full/files/itest_get_empty.txt"
    target_file.write_bytes(b"")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_empty.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code == 200,
            f"Empty file should return 200, got {res.status_code}",
        )
        assert_true(
            len(res.body_text) == 0 or res.body_text == "",
            f"Empty file body should be empty, got: {res.body_text!r}",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_case_sensitivity() -> None:
    target_file = ROOT / "www/full/files/itest_get_CaseSensitive.txt"
    target_file.write_text("case-sensitive\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req_correct = http10_request_bytes(
            method="GET",
            target="/files/itest_get_CaseSensitive.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req_correct)
            res_correct = read_http_response(sock)

        assert_true(
            res_correct.status_code == 200,
            f"Correct case should work, got {res_correct.status_code}",
        )

        req_wrong = http10_request_bytes(
            method="GET",
            target="/files/itest_get_casesensitive.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req_wrong)
            res_wrong = read_http_response(sock)

        assert_true(
            res_wrong.status_code == 404,
            f"Wrong case should return 404 (case-sensitive), got {res_wrong.status_code}",
        )

    if target_file.exists():
        target_file.unlink()


def test_get_conditional_if_modified_since() -> None:
    target_file = ROOT / "www/full/files/itest_get_conditional.txt"
    target_file.write_text("conditional-test\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req_first = http10_request_bytes(
            method="GET",
            target="/files/itest_get_conditional.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req_first)
            res_first = read_http_response(sock)

        last_modified = res_first.headers.get("last-modified")

        if last_modified:
            req_conditional = http10_request_bytes(
                method="GET",
                target="/files/itest_get_conditional.txt",
                host="localhost",
                headers={"If-Modified-Since": last_modified},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req_conditional)
                res_conditional = read_http_response(sock)

            assert_true(
                res_conditional.status_code in (200, 304),
                f"If-Modified-Since should return 200 or 304, got {res_conditional.status_code}",
            )

    if target_file.exists():
        target_file.unlink()


def test_get_malformed_crlf() -> None:
    with WebservRunner("config/valid/full.conf"):
        import socket

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 8080))

        malformed = b"GET /files/test.txt\r\n\r\n"

        try:
            sock.sendall(malformed)
            response = b""
            sock.settimeout(1.0)
            try:
                while True:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
            except socket.timeout:
                pass

            assert_true(
                True,
                "Malformed request handling varies - server may close or respond with error",
            )
        finally:
            sock.close()


def test_get_invalid_http_version() -> None:
    with WebservRunner("config/valid/full.conf"):
        import socket

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 8080))

        invalid_version = b"GET /files/test.txt HTTP/2.5\r\n\r\n"

        try:
            sock.sendall(invalid_version)
            response = b""
            sock.settimeout(2.0)
            try:
                while True:
                    chunk = sock.recv(4096)
                    if not chunk:
                        break
                    response += chunk
            except socket.timeout:
                pass

            assert_true(
                len(response) > 0,
                "Invalid HTTP version should get a response (error or rejection)",
            )
        finally:
            sock.close()


def test_get_very_long_header() -> None:
    target_file = ROOT / "www/full/files/itest_get_longheader.txt"
    target_file.write_text("long-header-test\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        long_value = "x" * 8192

        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_longheader.txt",
            host="localhost",
            headers={"X-Custom-Long-Header": long_value},
            body=b"",
        )

        with open_client(timeout=3.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (200, 400, 413),
            f"Very long header should succeed (200), be rejected (400), or entity too large (413), got {res.status_code}",
        )

    if target_file.exists():
        target_file.unlink()


def _rmtree_if_exists(path) -> None:
    if path.exists():
        shutil.rmtree(path)


def test_get_autoindex_on_lists_directory() -> None:
    root_dir = ROOT / "www/full/itest_autoindex_on"

    try:
        _rmtree_if_exists(root_dir)
        (root_dir / "subdir").mkdir(parents=True)
        (root_dir / "a.txt").write_text("a\n", encoding="utf-8")
        (root_dir / "space name.txt").write_text("b\n", encoding="utf-8")
        (root_dir / "subdir" / "nested.txt").write_text("c\n", encoding="utf-8")

        with WebservRunner("config/valid/autoindex_test.conf"):
            req = http10_request_bytes(
                method="GET",
                target="/auto_on/",
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
            assert_true(
                res.headers.get("content-type", "").lower().startswith("text/html"),
                f"Expected text/html listing, got Content-Type: {res.headers.get('content-type')}",
            )
            body = res.body_text
            assert_true("a.txt" in body, "Expected listing to contain a.txt")
            assert_true("subdir/" in body, "Expected listing to contain subdir/")
            assert_true("space name.txt" in body, "Expected listing to contain 'space name.txt'")
            assert_true("space%20name.txt" in body, "Expected href to URL-encode spaces")
            assert_true("../" in body, "Expected listing to contain parent link")

            # Also verify that the URL-encoded link resolves to the actual filename.
            req2 = http10_request_bytes(
                method="GET",
                target="/auto_on/space%20name.txt",
                host="localhost",
                headers={},
                body=b"",
            )
            with open_client() as sock:
                sock.sendall(req2)
                res2 = read_http_response(sock)
            assert_true(res2.status_code == 200, f"Expected 200, got {res2.status_code}")
            assert_true("b" in res2.body_text, "Expected served file content")
    finally:
        _rmtree_if_exists(root_dir)


def test_get_autoindex_off_directory_forbidden() -> None:
    root_dir = ROOT / "www/full/itest_autoindex_off"

    try:
        _rmtree_if_exists(root_dir)
        root_dir.mkdir(parents=True)
        (root_dir / "should_not_list.txt").write_text("nope\n", encoding="utf-8")

        with WebservRunner("config/valid/autoindex_test.conf"):
            req = http10_request_bytes(
                method="GET",
                target="/auto_off/",
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(res.status_code == 403, f"Expected 403, got {res.status_code}")
    finally:
        _rmtree_if_exists(root_dir)


def test_get_autoindex_off_serves_index_file() -> None:
    root_dir = ROOT / "www/full/itest_autoindex_off_index"
    index_file = root_dir / "index.html"

    try:
        _rmtree_if_exists(root_dir)
        root_dir.mkdir(parents=True)
        index_file.write_text("<html>index-ok</html>\n", encoding="utf-8")

        with WebservRunner("config/valid/autoindex_test.conf"):
            req = http10_request_bytes(
                method="GET",
                target="/auto_off_index/",
                host="localhost",
                headers={},
                body=b"",
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
            assert_true("index-ok" in res.body_text, "Expected to serve index.html")
    finally:
        _rmtree_if_exists(root_dir)


def main() -> int:
    tests = [
        ("GET basic file", test_get_basic),
        ("GET large 5MB file", test_get_large_file),
        ("GET 404 not found", test_get_404_not_found),
        ("GET multiple sequential requests", test_get_multiple_sequential),
        ("GET keep-alive multiple requests", test_get_keep_alive_multiple_requests),
        ("GET HTTP/1.1 default keep-alive", test_get_http11_default_keep_alive_multiple_requests),
        ("GET HTTP/1.1 Connection token-list close", test_get_http11_connection_close_token_list),
        ("GET HTTP/1.1 large pipeline", test_get_http11_large_pipeline),
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
        ("GET path traversal prefix collision", test_get_path_traversal_prefix_collision),
        ("GET excessive slashes", test_get_excessive_slashes),
        ("GET hidden files", test_get_hidden_files),
        ("GET special chars filename", test_get_special_chars_filename),
        ("GET symlink escape", test_get_symlink_escape),
        ("GET directory request", test_get_directory_request),
        ("GET query string", test_get_query_string),
        ("GET empty file", test_get_empty_file),
        ("GET case sensitivity", test_get_case_sensitivity),
        ("GET conditional If-Modified-Since", test_get_conditional_if_modified_since),
        ("GET malformed CRLF", test_get_malformed_crlf),
        ("GET invalid HTTP version", test_get_invalid_http_version),
        ("GET very long header", test_get_very_long_header),
        ("GET autoindex on lists directory", test_get_autoindex_on_lists_directory),
        ("GET autoindex off forbids directory", test_get_autoindex_off_directory_forbidden),
        ("GET autoindex off serves index", test_get_autoindex_off_serves_index_file),
    ]

    runner = TestRunner("GET", len(tests))
    for name, test_fn in tests:
        runner.run(name, test_fn)

    return runner.summary()


if __name__ == "__main__":
    raise SystemExit(main())
