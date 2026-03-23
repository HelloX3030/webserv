#include "test_harness.hpp"
#include "test_helpers.hpp"

/* body phase tests.
Content-Length path: accumulation across multiple advance() calls.
chunked path (RFC 9112 §7.1): sub-state machine exercised through
single chunks, multiple chunks, zero-only, extensions, errors.

all tests provide a valid request line + headers to reach BODY.
the prefix is constant; only body content varies. */

static const size_t MAX_BODY = 1048576;

static const std::string PREFIX =
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n";

static const std::string CHUNKED_PREFIX =
    "GET / HTTP/1.1\r\n"
    "Host: localhost\r\n"
    "Transfer-Encoding: chunked\r\n"
    "\r\n";

static const std::string TERM = "\r\n";


// --- Content-Length: partial delivery ---

TEST(body_cl_partial_delivery)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string headers = PREFIX + "Content-Length: 10\r\n\r\n";

    ParseResult r = advance_all(fe, headers);
    assert_eq(ParseStatus::Incomplete, r.status);

    r = fe.advance("hello", 5);
    assert_eq(ParseStatus::Incomplete, r.status);

    r = fe.advance("world", 5);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("helloworld"), r.request.body);
}

TEST(body_cl_single_byte_delivery)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = PREFIX + "Content-Length: 3\r\n\r\nabc";
    ParseResult r = advance_byte_by_byte(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("abc"), r.request.body);
}

TEST(body_cl_exact_boundary)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = PREFIX
        + "Content-Length: 4\r\n\r\n"
        + "data";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("data"), r.request.body);
}

TEST(body_cl_binary_bytes)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string headers = PREFIX + "Content-Length: 4\r\n\r\n";
    std::string body;
    body += '\x00';
    body += '\xff';
    body += '\r';
    body += '\n';
    ParseResult r = advance_all(fe, headers + body);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(size_t(4), r.request.body.size());
    assert_eq('\x00', r.request.body[0]);
    assert_eq('\xff', r.request.body[1]);
    assert_eq('\r', r.request.body[2]);
    assert_eq('\n', r.request.body[3]);
}


// --- chunked: valid sequences ---

TEST(body_chunked_single_chunk)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX
        + "5\r\nhello\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello"), r.request.body);
}

TEST(body_chunked_multiple_chunks)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX
        + "5\r\nhello\r\n"
        + "6\r\n world\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello world"), r.request.body);
}

TEST(body_chunked_empty_body)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string(""), r.request.body);
}

TEST(body_chunked_hex_uppercase)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX
        + "A\r\n0123456789\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("0123456789"), r.request.body);
}

TEST(body_chunked_hex_lowercase)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX
        + "a\r\n0123456789\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("0123456789"), r.request.body);
}

TEST(body_chunked_extension_stripped)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX
        + "5;ext=val\r\nhello\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello"), r.request.body);
}

TEST(body_chunked_byte_by_byte)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX
        + "3\r\nabc\r\n"
        + "2\r\nde\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_byte_by_byte(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("abcde"), r.request.body);
}

TEST(body_chunked_partial_delivery)
{
    HttpRequestFrontend fe(MAX_BODY);

    // send headers + first chunk size line
    std::string part1 = CHUNKED_PREFIX + "5\r\n";
    ParseResult r = advance_all(fe, part1);
    assert_eq(ParseStatus::Incomplete, r.status);

    // send chunk data + trailing CRLF
    r = fe.advance("hello\r\n", 7);
    assert_eq(ParseStatus::Incomplete, r.status);

    // send zero-chunk + trailer
    r = fe.advance("0\r\n\r\n", 5);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello"), r.request.body);
}


// --- chunked: errors ---

TEST(body_chunked_malformed_hex)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX + "xyz\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(body_chunked_empty_size_line)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = CHUNKED_PREFIX + "\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(body_chunked_missing_data_crlf)
{
    HttpRequestFrontend fe(MAX_BODY);
    // chunk says 5 bytes, we provide 5 bytes + "XX" instead of CRLF
    std::string input = CHUNKED_PREFIX + "5\r\nhelloXX";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

TEST(body_chunked_exceeds_max)
{
    HttpRequestFrontend fe(16); // 16-byte body limit
    std::string input = CHUNKED_PREFIX
        + "10\r\n0123456789abcdef\r\n"  // 16 bytes, at limit
        + "1\r\nX\r\n"                   // 17th byte: exceeds
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(413), r.error_code);
}

TEST(body_chunked_single_chunk_exceeds_max)
{
    HttpRequestFrontend fe(4);
    std::string input = CHUNKED_PREFIX
        + "5\r\nhello\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(413), r.error_code);
}

TEST(body_chunked_bad_trailer_crlf)
{
    HttpRequestFrontend fe(MAX_BODY);
    // zero-chunk followed by "XX" instead of CRLF
    std::string input = CHUNKED_PREFIX + "0\r\nXX";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Failed, r.status);
    assert_eq(uint16_t(400), r.error_code);
}

// ── edge cases: encoding interaction, hex variants ──

TEST(body_te_not_chunked_falls_through)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = PREFIX
        + "Transfer-Encoding: gzip\r\n"
        + "Content-Length: 3\r\n"
        + TERM
        + "abc";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("abc"), r.request.body);
}

TEST(body_te_chunked_overrides_cl)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = PREFIX
        + "Content-Length: 999\r\n"
        + "Transfer-Encoding: chunked\r\n"
        + TERM
        + "5\r\nhello\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello"), r.request.body);
}

TEST(body_chunked_multi_digit_hex)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string data(26, 'x');
    std::string input = PREFIX
        + "Transfer-Encoding: chunked\r\n"
        + TERM
        + "1a\r\n" + data + "\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(size_t(26), r.request.body.size());
    assert_eq(data, r.request.body);
}

TEST(body_chunked_leading_zeros)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string input = PREFIX
        + "Transfer-Encoding: chunked\r\n"
        + TERM
        + "005\r\nhello\r\n"
        + "0\r\n\r\n";
    ParseResult r = advance_all(fe, input);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("hello"), r.request.body);
}

// --- pipelining: reset + second request from residual buffer ---

TEST(pipelining_content_length)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string first = PREFIX
        + "Content-Length: 2\r\n\r\n"
        + "ab";
    std::string second =
        "POST /next HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    // feed both requests in one call
    ParseResult r = advance_all(fe, first + second);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("ab"), r.request.body);
    assert_eq(std::string("GET"), r.request.method);

    fe.reset();

    // second request should parse from residual buffer
    r = fe.advance("", 0);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("POST"), r.request.method);
    assert_eq(std::string("/next"), r.request.uri);
}

TEST(pipelining_chunked)
{
    HttpRequestFrontend fe(MAX_BODY);
    std::string first = CHUNKED_PREFIX
        + "3\r\nabc\r\n"
        + "0\r\n\r\n";
    std::string second =
        "DELETE /old HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ParseResult r = advance_all(fe, first + second);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("abc"), r.request.body);

    fe.reset();

    r = fe.advance("", 0);
    assert_eq(ParseStatus::Complete, r.status);
    assert_eq(std::string("DELETE"), r.request.method);
    assert_eq(std::string("/old"), r.request.uri);
}
