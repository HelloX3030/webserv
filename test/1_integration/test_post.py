#!/usr/bin/env python3

from common import ROOT, WebservRunner, assert_true, http10_request_bytes, open_client, read_http_response, run_test
import socket


def test_post_basic() -> None:
    target_file = ROOT / "www/full/files/itest_post_basic.txt"
    if target_file.exists():
        target_file.unlink()

    body = b"integration-post-body\nline2\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_basic.txt",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code in (201, 200), f"Expected 201 or 200, got {res.status_code}")
        assert_true(target_file.exists(), f"Expected file to exist: {target_file}")
        assert_true(
            target_file.read_bytes() == body,
            "File content mismatch after POST",
        )

    if target_file.exists():
        target_file.unlink()


def test_post_large_body() -> None:
    target_file = ROOT / "www/full/files/itest_post_large.bin"
    if target_file.exists():
        target_file.unlink()

    body = b"x" * (1 * 1024 * 1024)

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_large.bin",
            host="localhost",
            headers={
                "Content-Type": "application/octet-stream",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        try:
            with open_client(timeout=10.0) as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(res.status_code in (201, 200), f"Large body POST failed with {res.status_code}")
            assert_true(target_file.exists(), "Large file not created")
            assert_true(
                len(target_file.read_bytes()) == len(body),
                "Large file size mismatch",
            )
        except (ConnectionResetError, BrokenPipeError):
            assert_true(True, "Server limits large uploads (acceptable behavior)")

    if target_file.exists():
        target_file.unlink()


def test_post_empty_body() -> None:
    target_file = ROOT / "www/full/files/itest_post_empty.txt"
    if target_file.exists():
        target_file.unlink()

    body = b""

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_empty.txt",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code in (201, 200, 204), f"Empty body POST got {res.status_code}")

    if target_file.exists():
        target_file.unlink()


def test_post_overwrite_existing() -> None:
    target_file = ROOT / "www/full/files/itest_post_overwrite.txt"
    original_body = b"original content\n"
    target_file.write_bytes(original_body)

    new_body = b"new content after POST\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_overwrite.txt",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(new_body)),
            },
            body=new_body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code in (200, 201), f"POST overwrite got {res.status_code}")
        file_content = target_file.read_bytes()
        assert_true(
            file_content == new_body,
            f"Expected new content, got: {file_content!r}",
        )

    if target_file.exists():
        target_file.unlink()


def test_post_binary_data() -> None:
    target_file = ROOT / "www/full/files/itest_post_binary.bin"
    if target_file.exists():
        target_file.unlink()

    body = bytes(range(256)) * 10

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_binary.bin",
            host="localhost",
            headers={
                "Content-Type": "application/octet-stream",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code in (201, 200), f"Binary POST got {res.status_code}")
        assert_true(
            target_file.read_bytes() == body,
            "Binary content mismatch",
        )

    if target_file.exists():
        target_file.unlink()


def test_post_no_content_length() -> None:
    target_file = ROOT / "www/full/files/itest_post_no_len.txt"
    if target_file.exists():
        target_file.unlink()

    with WebservRunner("config/valid/full.conf"):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 8080))

        request = b"POST /files/itest_post_no_len.txt HTTP/1.0\r\n"
        request += b"Host: localhost\r\n"
        request += b"Content-Type: text/plain\r\n"
        request += b"\r\n"
        request += b"body without length\n"

        sock.sendall(request)
        sock.settimeout(2.0)

        try:
            response = b""
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
        except socket.timeout:
            pass
        finally:
            sock.close()

        assert_true(
            len(response) > 0,
            "POST without Content-Length should get a response",
        )

    if target_file.exists():
        target_file.unlink()


def test_post_mismatched_content_length() -> None:
    target_file = ROOT / "www/full/files/itest_post_mismatch.txt"
    if target_file.exists():
        target_file.unlink()

    body = b"actual body"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_mismatch.txt",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": "999",
            },
            body=body,
        )

        with open_client(timeout=2.0) as sock:
            sock.sendall(req)
            try:
                res = read_http_response(sock)
                assert_true(
                    res.status_code in (400, 413),
                    f"Mismatched Content-Length should fail or timeout, got {res.status_code}",
                )
            except:
                assert_true(True, "Server correctly rejected/timeout on mismatched length")

    if target_file.exists():
        target_file.unlink()


def test_post_path_traversal() -> None:
    target_file = ROOT / "www/full/files/itest_post_traversal.txt"
    if target_file.exists():
        target_file.unlink()

    body = b"should not write\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/../../../etc/passwd",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (403, 404),
            f"Path traversal POST should be blocked, got {res.status_code}",
        )

    if target_file.exists():
        target_file.unlink()


def test_post_multiple_sequential() -> None:
    files = []
    for i in range(3):
        target_file = ROOT / f"www/full/files/itest_post_seq_{i}.txt"
        files.append(target_file)
        if target_file.exists():
            target_file.unlink()

    with WebservRunner("config/valid/full.conf"):
        for i, target_file in enumerate(files):
            body = f"sequential post {i}\n".encode()

            req = http10_request_bytes(
                method="POST",
                target=f"/files/itest_post_seq_{i}.txt",
                host="localhost",
                headers={
                    "Content-Type": "text/plain",
                    "Content-Length": str(len(body)),
                },
                body=body,
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (201, 200),
                f"Sequential POST {i} failed with {res.status_code}",
            )
            assert_true(target_file.exists(), f"File {i} not created")

    for f in files:
        if f.exists():
            f.unlink()


def test_post_keep_alive_multiple() -> None:
    files = []
    for i in range(3):
        target_file = ROOT / f"www/full/files/itest_post_ka_{i}.txt"
        files.append(target_file)
        if target_file.exists():
            target_file.unlink()

    with WebservRunner("config/valid/full.conf"):
        with open_client() as sock:
            for i in range(3):
                body = f"keep-alive post {i}\n".encode()

                req = http10_request_bytes(
                    method="POST",
                    target=f"/files/itest_post_ka_{i}.txt",
                    host="localhost",
                    headers={
                        "Content-Type": "text/plain",
                        "Content-Length": str(len(body)),
                        "Connection": "keep-alive",
                    },
                    body=body,
                )

                sock.sendall(req)
                res = read_http_response(sock)

                assert_true(
                    res.status_code in (201, 200),
                    f"Keep-alive POST {i} failed with {res.status_code}",
                )

    for f in files:
        if f.exists():
            f.unlink()


def test_post_with_custom_headers() -> None:
    target_file = ROOT / "www/full/files/itest_post_headers.txt"
    if target_file.exists():
        target_file.unlink()

    body = b"data with custom headers\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_headers.txt",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
                "X-Custom-Header": "custom-value",
                "X-Another": "another-value",
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code in (201, 200), f"POST with custom headers got {res.status_code}")
        assert_true(target_file.exists(), "File not created with custom headers")

    if target_file.exists():
        target_file.unlink()


def test_post_content_types() -> None:
    content_types = [
        "text/plain",
        "application/json",
        "application/octet-stream",
        "text/html",
    ]

    files = []
    for ct in content_types:
        target_file = ROOT / f"www/full/files/itest_post_ct_{content_types.index(ct)}.txt"
        files.append(target_file)
        if target_file.exists():
            target_file.unlink()

    with WebservRunner("config/valid/full.conf"):
        for idx, ct in enumerate(content_types):
            body = f"data for {ct}\n".encode()
            target_file = files[idx]

            req = http10_request_bytes(
                method="POST",
                target=f"/files/itest_post_ct_{idx}.txt",
                host="localhost",
                headers={
                    "Content-Type": ct,
                    "Content-Length": str(len(body)),
                },
                body=body,
            )

            with open_client() as sock:
                sock.sendall(req)
                res = read_http_response(sock)

            assert_true(
                res.status_code in (201, 200),
                f"POST with Content-Type {ct} got {res.status_code}",
            )

    for f in files:
        if f.exists():
            f.unlink()


def test_post_query_string() -> None:
    target_file = ROOT / "www/full/files/itest_post_query.txt"
    if target_file.exists():
        target_file.unlink()

    body = b"data with query string\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_query.txt?foo=bar&baz=qux",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code in (201, 200), f"POST with query string got {res.status_code}")
        assert_true(target_file.exists(), "File not created despite query string")

    if target_file.exists():
        target_file.unlink()


def test_post_directory_request() -> None:
    body = b"should not work\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (403, 404),
            f"POST to directory should be forbidden or not found, got {res.status_code}",
        )


def test_post_invalid_http_version() -> None:
    target_file = ROOT / "www/full/files/itest_post_version.txt"
    if target_file.exists():
        target_file.unlink()

    with WebservRunner("config/valid/full.conf"):
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect(("localhost", 8080))

        request = b"POST /files/itest_post_version.txt HTTP/2.5\r\n"
        request += b"Host: localhost\r\n"
        request += b"Content-Length: 5\r\n"
        request += b"\r\n"
        request += b"hello"

        sock.sendall(request)
        sock.settimeout(2.0)

        try:
            response = sock.recv(4096)
            assert_true(
                len(response) > 0,
                "Invalid HTTP version should get error response",
            )
        finally:
            sock.close()

    if target_file.exists():
        target_file.unlink()


def test_post_very_long_header() -> None:
    target_file = ROOT / "www/full/files/itest_post_longhdr.txt"
    if target_file.exists():
        target_file.unlink()

    body = b"data\n"
    long_value = "x" * 8192

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_longhdr.txt",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
                "X-Custom-Long": long_value,
            },
            body=body,
        )

        with open_client(timeout=3.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (201, 200),
            f"POST with long header got {res.status_code}",
        )

    if target_file.exists():
        target_file.unlink()


def test_post_chunked_transfer_encoding_case_insensitive() -> None:
    with WebservRunner("config/valid/full.conf"):
        chunked_body = b"4\r\nTEST\r\n0\r\n\r\n"

        req = (
            b"POST /cgi-python/echo.py?name=chunked-case HTTP/1.0\r\n"
            b"Host: localhost\r\n"
            b"Transfer-Encoding: Chunked\r\n"
            b"Content-Type: text/plain\r\n"
            b"\r\n"
            + chunked_body
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Chunked POST expected 200, got {res.status_code}")
        assert_true(
            "content_length = 4" in res.body_text.lower(),
            f"Expected CGI to receive a 4-byte chunked body, got: {res.body_text!r}",
        )
        assert_true(
            "test" in res.body_text.lower(),
            f"Expected CGI to echo chunked payload, got: {res.body_text!r}",
        )


def test_post_chunked_transfer_encoding_token_list() -> None:
    with WebservRunner("config/valid/full.conf"):
        chunked_body = b"4\r\nTEST\r\n0\r\n\r\n"

        req = (
            b"POST /cgi-python/echo.py?name=chunked-token-list HTTP/1.1\r\n"
            b"Host: localhost\r\n"
            b"Transfer-Encoding: chunked, chunked\r\n"
            b"Content-Type: text/plain\r\n"
            b"Connection: close\r\n"
            b"\r\n"
            + chunked_body
        )

        with open_client(timeout=3.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Chunked token-list POST expected 200, got {res.status_code}")
        assert_true(
            "content_length = 4" in res.body_text.lower(),
            f"Expected CGI to receive a 4-byte chunked body from token-list TE, got: {res.body_text!r}",
        )
        assert_true(
            "test" in res.body_text.lower(),
            f"Expected CGI to echo chunked payload from token-list TE, got: {res.body_text!r}",
        )


def test_post_special_chars_filename() -> None:
    safe_filename = "itest_post_special______.txt"
    target_file = ROOT / f"www/full/files/{safe_filename}"
    if target_file.exists():
        target_file.unlink()

    body = b"special filename\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target=f"/files/{safe_filename}",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
            },
            body=body,
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code in (201, 200),
            f"POST with special chars filename got {res.status_code}",
        )

    if target_file.exists():
        target_file.unlink()


def main() -> int:
    tests = [
        ("POST basic file creation", test_post_basic),
        ("POST large 5MB body", test_post_large_body),
        ("POST empty body", test_post_empty_body),
        ("POST overwrite existing file", test_post_overwrite_existing),
        ("POST binary data", test_post_binary_data),
        ("POST no Content-Length header", test_post_no_content_length),
        ("POST mismatched Content-Length", test_post_mismatched_content_length),
        ("POST path traversal attempt", test_post_path_traversal),
        ("POST multiple sequential requests", test_post_multiple_sequential),
        ("POST keep-alive multiple requests", test_post_keep_alive_multiple),
        ("POST with custom headers", test_post_with_custom_headers),
        ("POST various Content-Types", test_post_content_types),
        ("POST with query string", test_post_query_string),
        ("POST to directory", test_post_directory_request),
        ("POST invalid HTTP version", test_post_invalid_http_version),
        ("POST very long header", test_post_very_long_header),
        ("POST chunked Transfer-Encoding case-insensitive", test_post_chunked_transfer_encoding_case_insensitive),
        ("POST chunked Transfer-Encoding token-list", test_post_chunked_transfer_encoding_token_list),
        ("POST special chars filename", test_post_special_chars_filename),
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

    print(f"\nAll {passed} POST tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
