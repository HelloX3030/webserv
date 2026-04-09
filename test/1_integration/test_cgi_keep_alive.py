#!/usr/bin/env python3

from common import ROOT, WebservRunner, assert_true, http10_request_bytes, open_client, read_http_response, run_test


def test_cgi_keep_alive_http10() -> None:
    target_file = ROOT / "www/full/files/itest_keep_alive.txt"
    target_file.write_text("hello from keep-alive fixture\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        with open_client(timeout=3.0) as sock:
            req1 = http10_request_bytes(
                method="GET",
                target="/cgi-python/hello.py?name=keepalive",
                host="localhost",
                headers={"Connection": "keep-alive"},
                body=b"",
            )
            sock.sendall(req1)
            res1 = read_http_response(sock)

            assert_true(res1.status_code == 200, f"First response expected 200, got {res1.status_code}")
            assert_true(
                "hello from full/python cgi" in res1.body_text.lower(),
                f"Unexpected first body: {res1.body_text!r}",
            )
            assert_true(
                res1.headers.get("connection", "").lower() == "keep-alive",
                f"Expected keep-alive on first response, got {res1.headers.get('connection')}",
            )

            req2 = http10_request_bytes(
                method="GET",
                target="/files/itest_keep_alive.txt",
                host="localhost",
                headers={"Connection": "close"},
                body=b"",
            )
            sock.sendall(req2)
            res2 = read_http_response(sock)

            assert_true(res2.status_code == 200, f"Second response expected 200, got {res2.status_code}")
            assert_true(
                "hello" in res2.body_text.lower(),
                f"Unexpected second body: {res2.body_text!r}",
            )
            assert_true(
                res2.headers.get("connection", "").lower() == "close",
                f"Expected close on second response, got {res2.headers.get('connection')}",
            )

    if target_file.exists():
        target_file.unlink()


def main() -> int:
    return run_test("CGI + keep-alive over HTTP/1.0", test_cgi_keep_alive_http10)


if __name__ == "__main__":
    raise SystemExit(main())
