#!/usr/bin/env python3

from pathlib import Path

from common import ROOT, WebservRunner, TestRunner, assert_true, open_client, read_http_response


def _send_raw_and_get_status(raw_request: bytes, config_rel_path: str = "config/valid/full.conf") -> int:
    with WebservRunner(config_rel_path):
        with open_client(timeout=3.0) as sock:
            sock.sendall(raw_request)
            res = read_http_response(sock)
            return res.status_code


def test_invalid_request_line_missing_version() -> None:
    req = (
        b"GET /files/itest_invalid_http.txt\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"Missing HTTP version should return 400, got {status}")


def test_invalid_http_version_unsupported() -> None:
    req = (
        b"GET /files/itest_invalid_http.txt HTTP/2.0\r\n"
        b"Host: localhost\r\n"
        b"\r\n"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 505, f"Unsupported HTTP version should return 505, got {status}")


def test_invalid_method_not_implemented() -> None:
    req = (
        b"PUT /files/itest_invalid_http_put.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Length: 3\r\n"
        b"\r\n"
        b"abc"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 501, f"Unsupported method should return 501, got {status}")


def test_invalid_http11_missing_host() -> None:
    req = (
        b"GET /files/itest_invalid_http.txt HTTP/1.1\r\n"
        b"Connection: close\r\n"
        b"\r\n"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"HTTP/1.1 without Host should return 400, got {status}")


def test_invalid_malformed_header_line() -> None:
    req = (
        b"GET /files/itest_invalid_http.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"BadHeaderWithoutColon\r\n"
        b"\r\n"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"Malformed header line should return 400, got {status}")


def test_invalid_content_length_non_numeric() -> None:
    target_file = ROOT / "www/full/files/itest_invalid_http_non_numeric.txt"
    if target_file.exists():
        target_file.unlink()

    req = (
        b"POST /files/itest_invalid_http_non_numeric.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: not-a-number\r\n"
        b"\r\n"
        b"payload"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"Non-numeric Content-Length should return 400, got {status}")
    assert_true(not target_file.exists(), "File should not be created for invalid Content-Length")


def test_invalid_duplicate_content_length_different_values() -> None:
    target_file = ROOT / "www/full/files/itest_invalid_http_dup_cl.txt"
    if target_file.exists():
        target_file.unlink()

    req = (
        b"POST /files/itest_invalid_http_dup_cl.txt HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Content-Type: text/plain\r\n"
        b"Content-Length: 3\r\n"
        b"Content-Length: 10\r\n"
        b"\r\n"
        b"abc"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"Differing duplicate Content-Length should return 400, got {status}")
    assert_true(not target_file.exists(), "File should not be created for duplicate differing Content-Length")


def test_invalid_duplicate_host_header() -> None:
    req = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Host: localhost\r\n"
        b"Connection: close\r\n"
        b"\r\n"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"Duplicate Host should return 400, got {status}")


def test_invalid_chunked_bad_hex_size() -> None:
    req = (
        b"POST /cgi-python/echo.py HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"Connection: close\r\n"
        b"\r\n"
        b"ZZ\r\n"
        b"boom\r\n"
        b"0\r\n\r\n"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"Invalid chunk-size should return 400, got {status}")


def test_invalid_chunked_missing_data_crlf() -> None:
    req = (
        b"POST /cgi-python/echo.py HTTP/1.1\r\n"
        b"Host: localhost\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"Connection: close\r\n"
        b"\r\n"
        b"4\r\n"
        b"TESTXX"
    )
    status = _send_raw_and_get_status(req)
    assert_true(status == 400, f"Chunk-data without CRLF terminator should return 400, got {status}")


def main() -> int:
    tests = [
        ("Invalid request line missing version", test_invalid_request_line_missing_version),
        ("Invalid unsupported HTTP version", test_invalid_http_version_unsupported),
        ("Invalid method not implemented", test_invalid_method_not_implemented),
        ("Invalid HTTP/1.1 missing Host", test_invalid_http11_missing_host),
        ("Invalid malformed header line", test_invalid_malformed_header_line),
        ("Invalid non-numeric Content-Length", test_invalid_content_length_non_numeric),
        ("Invalid duplicate differing Content-Length", test_invalid_duplicate_content_length_different_values),
        ("Invalid duplicate Host header", test_invalid_duplicate_host_header),
        ("Invalid chunked bad hex size", test_invalid_chunked_bad_hex_size),
        ("Invalid chunked missing data CRLF", test_invalid_chunked_missing_data_crlf),
    ]

    runner = TestRunner("Invalid HTTP", len(tests))
    for name, test_fn in tests:
        runner.run(name, test_fn)

    return runner.summary()


if __name__ == "__main__":
    raise SystemExit(main())
