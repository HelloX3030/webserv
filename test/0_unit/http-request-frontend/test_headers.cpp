#include "test_harness.hpp"
#include "test_helpers.hpp"

/* headers phase tests.
RFC 9110 section 5: field-line = field-name ":" OWS field-value OWS
RFC 9112 section 3.2: HTTP/1.1 requires Host header.

all tests provide a valid request line first — the parser must
pass through REQUEST_LINE to reach HEADERS. the request line
is constant; only the header section varies. */

static const size_t MAX_BODY = 1048576;

static const std::string RL = "GET / HTTP/1.1\r\n";
static const std::string RL_10 = "GET / HTTP/1.0\r\n";
static const std::string TERM = "\r\n"; // header section terminator


// --- field parsing ---

TEST(headers_single)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, RL + "Host: localhost\r\n" + TERM);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("localhost"), r.request.headers.at("host"));
}

TEST(headers_multiple)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Accept: text/html\r\n"
        + "X-Custom: value\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("localhost"), r.request.headers.at("host"));
    assert_eq(std::string("text/html"), r.request.headers.at("accept"));
    assert_eq(std::string("value"), r.request.headers.at("x-custom"));
}

TEST(headers_case_normalisation)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Type: text/plain\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("text/plain"), r.request.headers.at("content-type"));
}

TEST(headers_ows_trimmed)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host:   localhost  \r\n"
        + "Accept:\t text/html \t\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("localhost"), r.request.headers.at("host"));
    assert_eq(std::string("text/html"), r.request.headers.at("accept"));
}

TEST(headers_value_no_ows)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL + "Host:localhost\r\n" + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("localhost"), r.request.headers.at("host"));
}

TEST(headers_empty_value)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "X-Empty:\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string(""), r.request.headers.at("x-empty"));
}

TEST(headers_duplicate_last_wins)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: first\r\n"
        + "Host: second\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("second"), r.request.headers.at("host"));
}

TEST(headers_value_with_colon)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost:8080\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("localhost:8080"), r.request.headers.at("host"));
}

TEST(headers_byte_by_byte)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Accept: */*\r\n"
        + TERM;
    ParseResult r = advance_byte_by_byte(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("localhost"), r.request.headers.at("host"));
    assert_eq(std::string("*/*"), r.request.headers.at("accept"));
}


// --- Host enforcement ---

TEST(headers_host_missing_http11)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL + "Accept: text/html\r\n" + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(headers_host_missing_http10_ok)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL_10 + "Accept: text/html\r\n" + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
}

TEST(headers_empty_section_http10)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL_10 + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
}


// --- Content-Length handling ---

TEST(headers_content_length_zero)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Length: 0\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string(""), r.request.body);
}

TEST(headers_content_length_triggers_body)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Length: 5\r\n"
        + TERM
        + "hello";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello"), r.request.body);
}

TEST(headers_content_length_negative)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Length: -1\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(headers_content_length_non_numeric)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Length: abc\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(headers_content_length_trailing_garbage)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Length: 42abc\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(headers_content_length_exceeds_max)
{
    HttpRequestFrontend fe(64); // 64-byte limit
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Length: 65\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(413), r.error_code);
}

TEST(headers_content_length_at_max)
{
    HttpRequestFrontend fe(5);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Content-Length: 5\r\n"
        + TERM
        + "12345";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("12345"), r.request.body);
}


// --- Transfer-Encoding: chunked branching ---

TEST(headers_chunked_triggers_chunked_body)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Transfer-Encoding: chunked\r\n"
        + TERM
        + "5\r\nhello\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello"), r.request.body);
}


// --- no body indicators → COMPLETE with empty body ---

TEST(headers_no_body_indicators)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL + "Host: localhost\r\n" + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string(""), r.request.body);
}


// --- malformed field lines: 400 ---

TEST(headers_no_colon)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL + "InvalidHeader\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(headers_colon_at_start)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL + ": value\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}


// --- keepAlive derived correctly ---

TEST(headers_keepalive_http11_default)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, RL + "Host: localhost\r\n" + TERM);
    assert_eq(ParseStatus::Complete, r.status);
    ASSERT_TRUE(r.request.keepAlive());
}

TEST(headers_keepalive_http11_close)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL
        + "Host: localhost\r\n"
        + "Connection: close\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    ASSERT_TRUE(!r.request.keepAlive());
}

TEST(headers_keepalive_http10_default)
{
    HttpRequestFrontend fe(MAX_BODY);
    ParseResult r = advance_all(fe, RL_10 + TERM);
    assert_eq(ParseStatus::Complete, r.status);
    ASSERT_TRUE(!r.request.keepAlive());
}

TEST(headers_keepalive_http10_keepalive)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = RL_10
        + "Connection: keep-alive\r\n"
        + TERM;
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    ASSERT_TRUE(r.request.keepAlive());
}
