#!/usr/bin/env python3

from common import WebservRunner, TestRunner, assert_true, http10_request_bytes, open_client, read_http_response


def fetch_index_for_host(host_header: str | None, target: str = "/") -> str:
    headers = {}
    if host_header is not None:
        headers = {"Connection": "close"}

    req = http10_request_bytes(
        method="GET",
        target=target,
        host=host_header if host_header is not None else "localhost",
        headers=headers,
        body=b"",
    )

    if host_header is None:
        req = req.replace(b"Host: localhost\r\n", b"")

    with open_client() as sock:
        sock.sendall(req)
        res = read_http_response(sock)

    assert_true(res.status_code == 200, f"Expected 200 for host={host_header}, got {res.status_code}")
    return res.body_text.lower()


def test_virtual_hosting_alpha_beta() -> None:
    with WebservRunner("config/valid/multi-server.conf"):
        alpha_body = fetch_index_for_host("alpha.localhost")
        beta_body = fetch_index_for_host("beta.localhost")

        assert_true("multi-server.conf: alpha" in alpha_body, "alpha host did not route to alpha content")
        assert_true("multi-server.conf: beta" in beta_body, "beta host did not route to beta content")


def test_virtual_hosting_default_fallback() -> None:
    with WebservRunner("config/valid/multi-server.conf"):
        fallback_body = fetch_index_for_host("unknown.localhost")

        assert_true(
            "multi-server.conf: alpha" in fallback_body,
            "Unknown host should fall back to the first server block (alpha)",
        )


def test_virtual_hosting_host_with_port() -> None:
    with WebservRunner("config/valid/multi-server.conf"):
        alpha_body = fetch_index_for_host("alpha.localhost:8080")
        beta_body = fetch_index_for_host("beta.localhost:8080")

        assert_true("multi-server.conf: alpha" in alpha_body, "Host: alpha.localhost:8080 did not route to alpha")
        assert_true("multi-server.conf: beta" in beta_body, "Host: beta.localhost:8080 did not route to beta")


def test_virtual_hosting_missing_host_header() -> None:
    with WebservRunner("config/valid/multi-server.conf"):
        body = fetch_index_for_host(None)

        assert_true(
            "multi-server.conf: alpha" in body,
            "Missing Host header should use the default first server block (alpha)",
        )


def test_virtual_hosting_duplicate_host_header_rejected() -> None:
    with WebservRunner("config/valid/multi-server.conf"):
        req = (
            b"GET / HTTP/1.1\r\n"
            b"Host: alpha.localhost\r\n"
            b"Host: beta.localhost\r\n"
            b"Connection: close\r\n"
            b"\r\n"
        )

        with open_client() as sock:
            sock.sendall(req)
            res = read_http_response(sock)

        assert_true(
            res.status_code == 400,
            f"Duplicate Host headers must be rejected with 400, got {res.status_code}",
        )


def test_virtual_hosting_isolated_roots() -> None:
    with WebservRunner("config/valid/multi-server.conf"):
        alpha_body = fetch_index_for_host("alpha.localhost", target="/index.html")
        beta_body = fetch_index_for_host("beta.localhost", target="/index.html")

        assert_true("alpha.localhost" in alpha_body, "alpha root not isolated")
        assert_true("beta.localhost" in beta_body, "beta root not isolated")


def main() -> int:
    tests = [
        ("Virtual hosting routes alpha/beta", test_virtual_hosting_alpha_beta),
        ("Virtual hosting fallback host", test_virtual_hosting_default_fallback),
        ("Virtual hosting host:port normalization", test_virtual_hosting_host_with_port),
        ("Virtual hosting missing Host header", test_virtual_hosting_missing_host_header),
        ("Virtual hosting duplicate Host rejected", test_virtual_hosting_duplicate_host_header_rejected),
        ("Virtual hosting isolated roots", test_virtual_hosting_isolated_roots),
    ]

    runner = TestRunner("Virtual hosting", len(tests))
    for name, test_fn in tests:
        runner.run(name, test_fn)

    return runner.summary()


if __name__ == "__main__":
    raise SystemExit(main())
