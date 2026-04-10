#!/usr/bin/env python3

from common import ROOT, WebservRunner, TestRunner, assert_true, http10_request_bytes, open_client, read_http_response


def test_cgi_python_keep_alive_get_then_static() -> None:
    target_file = ROOT / "www/full/files/itest_cgi_keep_alive_static.txt"
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
                target="/files/itest_cgi_keep_alive_static.txt",
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


def test_cgi_python_keep_alive_post_echo() -> None:
    with WebservRunner("config/valid/full.conf"):
        body = b"alpha=1&beta=2\nline-two\n"

        with open_client(timeout=3.0) as sock:
            req1 = http10_request_bytes(
                method="POST",
                target="/cgi-python/echo.py?name=postkeepalive",
                host="localhost",
                headers={
                    "Connection": "keep-alive",
                    "Content-Type": "application/x-www-form-urlencoded",
                    "Content-Length": str(len(body)),
                },
                body=body,
            )
            sock.sendall(req1)
            res1 = read_http_response(sock)

            assert_true(res1.status_code == 200, f"POST CGI expected 200, got {res1.status_code}")
            assert_true(
                "echo from full/python cgi" in res1.body_text.lower(),
                f"Unexpected CGI body: {res1.body_text!r}",
            )
            assert_true(
                "request_method = post" in res1.body_text.lower(),
                f"Expected POST method in CGI output, got {res1.body_text!r}",
            )
            assert_true(
                "content_length =" in res1.body_text.lower(),
                f"Expected content length in CGI output, got {res1.body_text!r}",
            )
            assert_true(
                body.decode() in res1.body_text,
                f"Expected request body to be echoed, got {res1.body_text!r}",
            )
            assert_true(
                res1.headers.get("connection", "").lower() == "keep-alive",
                f"Expected keep-alive on CGI POST, got {res1.headers.get('connection')}",
            )


def test_cgi_python_env_dump_post() -> None:
    with WebservRunner("config/valid/full.conf"):
        body = b"payload-for-env-dump\n"

        req = http10_request_bytes(
            method="POST",
            target="/cgi-python/env.py?source=keepalive-test",
            host="localhost",
            headers={
                "Content-Type": "text/plain",
                "Content-Length": str(len(body)),
                "Connection": "keep-alive",
            },
            body=body,
        )

        with open_client(timeout=3.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Env dump expected 200, got {res.status_code}")
        assert_true("CGI env dump (python)" in res.body_text, f"Unexpected env dump: {res.body_text!r}")
        assert_true("REQUEST_METHOD=POST" in res.body_text, f"Missing POST method in env dump: {res.body_text!r}")
        assert_true(f"CONTENT_LENGTH={len(body)}" in res.body_text, f"Missing content length in env dump: {res.body_text!r}")
        assert_true("CONTENT_TYPE=text/plain" in res.body_text, f"Missing content type in env dump: {res.body_text!r}")


def test_cgi_bash_keep_alive_get_then_static() -> None:
    target_file = ROOT / "www/full/files/itest_cgi_keep_alive_bash.txt"
    target_file.write_text("hello from bash keep-alive fixture\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        with open_client(timeout=3.0) as sock:
            req1 = http10_request_bytes(
                method="GET",
                target="/cgi-bash/hello.sh?name=keepalive",
                host="localhost",
                headers={"Connection": "keep-alive"},
                body=b"",
            )
            sock.sendall(req1)
            res1 = read_http_response(sock)

            assert_true(res1.status_code == 200, f"First response expected 200, got {res1.status_code}")
            assert_true(
                "hello from full/bash cgi" in res1.body_text.lower(),
                f"Unexpected first body: {res1.body_text!r}",
            )
            assert_true(
                res1.headers.get("connection", "").lower() == "keep-alive",
                f"Expected keep-alive on first response, got {res1.headers.get('connection')}",
            )

            req2 = http10_request_bytes(
                method="GET",
                target="/files/itest_cgi_keep_alive_bash.txt",
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

    if target_file.exists():
        target_file.unlink()


def test_cgi_bash_env_dump_keep_alive() -> None:
    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="GET",
            target="/cgi-bash/env.sh?source=bash-keepalive",
            host="localhost",
            headers={"Connection": "keep-alive"},
            body=b"",
        )

        with open_client(timeout=3.0) as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Bash env dump expected 200, got {res.status_code}")
        assert_true("CGI env dump (bash)" in res.body_text, f"Unexpected env dump: {res.body_text!r}")
        assert_true("REQUEST_METHOD=GET" in res.body_text, f"Missing GET method in env dump: {res.body_text!r}")
        assert_true("SCRIPT_FILENAME=" in res.body_text, f"Missing script filename in env dump: {res.body_text!r}")


def test_cgi_bash_post_echo_keep_alive() -> None:
    with WebservRunner("config/valid/full.conf"):
        body = b"bash echo payload\nsecond line\n"

        with open_client(timeout=3.0) as sock:
            req = http10_request_bytes(
                method="POST",
                target="/cgi-bash/echo.sh",
                host="localhost",
                headers={
                    "Connection": "keep-alive",
                    "Content-Type": "text/plain",
                    "Content-Length": str(len(body)),
                },
                body=body,
            )
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Bash echo expected 200, got {res.status_code}")
        assert_true(body.decode() in res.body_text, f"Expected posted body to be echoed, got {res.body_text!r}")


def main() -> int:
    tests = [
        ("CGI python keep-alive GET then static", test_cgi_python_keep_alive_get_then_static),
        ("CGI python keep-alive POST echo", test_cgi_python_keep_alive_post_echo),
        ("CGI python env dump POST", test_cgi_python_env_dump_post),
        ("CGI bash keep-alive GET then static", test_cgi_bash_keep_alive_get_then_static),
        ("CGI bash env dump keep-alive", test_cgi_bash_env_dump_keep_alive),
        ("CGI bash POST echo keep-alive", test_cgi_bash_post_echo_keep_alive),
    ]

    runner = TestRunner("CGI keep-alive", len(tests))
    for name, test_fn in tests:
        runner.run(name, test_fn)

    return runner.summary()


if __name__ == "__main__":
    raise SystemExit(main())
