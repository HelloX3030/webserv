#!/usr/bin/env python3

from common import WebservRunner, assert_true, http10_request_bytes, open_client, read_http_response, run_test


def fetch_index_for_host(host_header: str) -> str:
    req = http10_request_bytes(
        method="GET",
        target="/",
        host=host_header,
        headers={},
        body=b"",
    )

    with open_client() as sock:
        sock.sendall(req)
        res = read_http_response(sock)

    assert_true(res.status_code == 200, f"Expected 200 for host={host_header}, got {res.status_code}")
    return res.body_text.lower()


def test_virtual_hosting() -> None:
    with WebservRunner("config/valid/multi-server.conf"):
        alpha_body = fetch_index_for_host("alpha.localhost")
        beta_body = fetch_index_for_host("beta.localhost")

        assert_true("alpha.localhost" in alpha_body, "alpha host did not route to alpha content")
        assert_true("beta.localhost" in beta_body, "beta host did not route to beta content")


def main() -> int:
    return run_test("Virtual hosting routes by Host header", test_virtual_hosting)


if __name__ == "__main__":
    raise SystemExit(main())
