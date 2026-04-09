#!/usr/bin/env python3

from common import ROOT, WebservRunner, assert_true, http10_request_bytes, open_client, read_http_response, run_test


def test_get() -> None:
    target_file = ROOT / "www/full/files/itest_get_http10.txt"
    target_file.write_text("hello from get fixture\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/files/itest_get_http10.txt",
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


def main() -> int:
    return run_test("GET HTTP/1.0 returns file and closes by default", test_get)


if __name__ == "__main__":
    raise SystemExit(main())
