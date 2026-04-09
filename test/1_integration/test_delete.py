#!/usr/bin/env python3

from common import ROOT, WebservRunner, assert_true, http10_request_bytes, open_client, read_http_response, run_test


def test_delete() -> None:
    target_file = ROOT / "www/full/files/itest_delete_http10.txt"
    target_file.write_text("delete-me\n", encoding="utf-8")

    with WebservRunner("config/valid/full.conf"):
        req = http10_request_bytes(
            method="DELETE",
            target="/files/itest_delete_http10.txt",
            host="localhost",
            headers={},
            body=b"",
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(res.status_code == 200, f"Expected 200, got {res.status_code}")
        assert_true(not target_file.exists(), f"Expected file to be deleted: {target_file}")


def main() -> int:
    return run_test("DELETE HTTP/1.0 removes target file", test_delete)


if __name__ == "__main__":
    raise SystemExit(main())
