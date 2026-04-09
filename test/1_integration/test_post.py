#!/usr/bin/env python3

from common import ROOT, WebservRunner, assert_true, http10_request_bytes, open_client, read_http_response, run_test


def test_post() -> None:
    target_file = ROOT / "www/full/files/itest_post_http10.txt"
    if target_file.exists():
        target_file.unlink()

    body = b"integration-post-body\nline2\n"

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="POST",
            target="/files/itest_post_http10.txt",
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

        assert_true(res.status_code == 201, f"Expected 201 Created, got {res.status_code}")
        assert_true(target_file.exists(), f"Expected file to exist: {target_file}")
        assert_true(
            target_file.read_bytes() == body,
            "File content mismatch after POST",
        )

    if target_file.exists():
        target_file.unlink()


def main() -> int:
    return run_test("POST HTTP/1.0 creates file with expected body", test_post)


if __name__ == "__main__":
    raise SystemExit(main())
