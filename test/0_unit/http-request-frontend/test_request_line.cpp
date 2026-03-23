#include "test_harness.hpp"
#include "test_helpers.hpp"

/* request-line phase tests.
RFC 9112 section 3: request-line = method SP request-target SP HTTP-version CRLF

valid requests: method must be GET/POST/DELETE, URI must begin with '/',
version must be HTTP/1.0 or HTTP/1.1. a complete minimal request
requires the header terminator (empty CRLF) and, for HTTP/1.1,
a Host header.

error tests need only the malformed request line — the parser
fails in REQUEST_LINE phase before reaching HEADERS. */

static const size_t MAX_BODY = 1048576; // 1 MiB, irrelevant for these tests

/* minimal valid request strings.
HTTP/1.1 requires Host (enforced at end of HEADERS phase).
HTTP/1.0 does not. */
static const std::string GET_11 =
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "\r\n";

static const std::string POST_11 =
    "POST /submit HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "\r\n";

static const std::string DELETE_11 =
    "DELETE /resource HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "\r\n";

static const std::string GET_10 =
    "GET /index.html HTTP/1.0\r\n"
    "\r\n";


// --- valid requests: fields populated correctly ---

TEST(request_line_get)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, GET_11);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("GET"), r.request.method);
    assert_eq(std::string("/"), r.request.uri);
    assert_eq(std::string("HTTP/1.1"), r.request.http_version);
}

TEST(request_line_post)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, POST_11);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("POST"), r.request.method);
    assert_eq(std::string("/submit"), r.request.uri);
}

TEST(request_line_delete)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, DELETE_11);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("DELETE"), r.request.method);
    assert_eq(std::string("/resource"), r.request.uri);
}

TEST(request_line_http10)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, GET_10);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("HTTP/1.0"), r.request.http_version);
}

TEST(request_line_uri_with_query)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input =
        "GET /path?key=value&a=b HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("/path?key=value&a=b"), r.request.uri);
}

TEST(request_line_uri_deep_path)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input =
        "GET /a/b/c/d HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("/a/b/c/d"), r.request.uri);
}


// --- valid request, byte-at-a-time delivery ---

TEST(request_line_byte_by_byte)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_byte_by_byte(fe, GET_11);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("GET"), r.request.method);
    assert_eq(std::string("/"), r.request.uri);
    assert_eq(std::string("HTTP/1.1"), r.request.http_version);
}


// --- incomplete: no CRLF yet ---

TEST(request_line_incomplete)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTP/1.1");
    assert_eq(ParseStatus::Incomplete, r.status);
}


// --- method errors: 501 ---

TEST(request_line_unknown_method)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "PUT / HTTP/1.1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(501), r.error_code);
}

TEST(request_line_head_method)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "HEAD / HTTP/1.1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(501), r.error_code);
}

TEST(request_line_method_case_sensitive)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "get / HTTP/1.1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(501), r.error_code);
}


// --- SP structure errors: 400 ---

TEST(request_line_no_sp)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(request_line_single_sp)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET /\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(request_line_trailing_sp)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTP/1.1 \r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(request_line_leading_sp)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, " GET / HTTP/1.1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(request_line_double_sp)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET  / HTTP/1.1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(request_line_empty)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}


// --- URI errors: 400 ---

TEST(request_line_uri_no_leading_slash)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET index.html HTTP/1.1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}


// --- version errors: 505 vs 400 ---

TEST(request_line_http20)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTP/2.0\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(505), r.error_code);
}

TEST(request_line_http30)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTP/3.0\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(505), r.error_code);
}

TEST(request_line_http09)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTP/0.9\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(505), r.error_code);
}

TEST(request_line_garbage_version)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTZ/1.1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(request_line_short_version)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTP\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(request_line_version_no_minor)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, "GET / HTTP/1\r\n");
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

/*
Trace of the error code logic the tests verify — the parser's decision procedure:

line has < 2 SP          → 400  (structural)
empty token              → 400  (structural)
extra SP in version      → 400  (structural)
method ∉ {GET,POST,DELETE} → 501  (not implemented)
uri[0] != '/'            → 400  (malformed)
version matches HTTP/D.D
  but D.D ∉ {1.0, 1.1}  → 505  (version not supported)
version does not match
  HTTP/D.D pattern       → 400  (malformed)
*/
