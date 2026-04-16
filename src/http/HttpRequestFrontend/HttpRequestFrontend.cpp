#include "http/HttpRequestFrontend.hpp"
#include "HttpRequestFrontend_internal.hpp"
#include <cassert>

/* construction.
max_body_size from server configuration — constant for connection lifetime.
all other fields initialised to their pre-parse state.
see state machine doc invariants 1–8. */
HttpRequestFrontend::HttpRequestFrontend(size_t max_body_size)
    :
    max_body_size_(max_body_size),
    phase_(ParsePhase::REQUEST_LINE),
    error_code_(0),
    buffer_()   ,
    request_(),
    header_bytes_(0),
    body_remaining_(0),
    body_chunked_(false),
    chunk_phase_(ChunkPhase::SIZE),
    chunk_remaining_(0)
{
}

/* append bytes to internal buffer, advance parse state.
returns as soon as status is determinable.

the while loop handles the case where a single call provides
enough bytes to complete multiple phases — including multiple
chunked sub-state transitions within a single BODY phase.
loop runs until NeedMore, Complete, or Failed.

each phase parser returns PhaseResult:
    Advanced → phase completed, loop continues to next phase.
    NeedMore → insufficient bytes, return Incomplete.
    Failed   → parse error, error_code_ set, return Failed. */
ParseResult HttpRequestFrontend::advance(const char* data, size_t len)
{
    assert(phase_ != ParsePhase::COMPLETE && "advance() called after parse complete");
    assert(phase_ != ParsePhase::ERROR && "advance() called after parse error");

    buffer_.append(data, len);

    while (true)
    {
        switch (phase_)
        {
            case ParsePhase::REQUEST_LINE:
            {
                PhaseResult r = parse_request_line();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                {
                    assert(phase_ == ParsePhase::ERROR);
                    assert(error_code_ != 0);
                    return {ParseStatus::Failed, {}, error_code_};
                }
                break;
            }

            case ParsePhase::HEADERS:
            {
                PhaseResult r = parse_header_line();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                {
                    assert(phase_ == ParsePhase::ERROR);
                    assert(error_code_ != 0);
                    return {ParseStatus::Failed, {}, error_code_};
                }
                break;
            }

            case ParsePhase::BODY:
            {
                PhaseResult r = consume_body();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                {
                    assert(phase_ == ParsePhase::ERROR);
                    assert(error_code_ != 0);
                    return {ParseStatus::Failed, {}, error_code_};
                }
                break;
            }

            case ParsePhase::COMPLETE:
                assert(error_code_ == 0);
                return {ParseStatus::Complete, std::move(request_), 0};

            case ParsePhase::ERROR:
                assert(false && "unreachable: ERROR phase at loop entry");
                return {ParseStatus::Failed, {}, error_code_};
        }
    }
}

/* clear parse state for next request on persistent connection.
called by Connection after response sent, iff keepAlive() is true.

buffer_ is preserved: may contain bytes from pipelined next request.
max_body_size_ is preserved: constant for connection lifetime.
chunked fields are reset defensively: always set fresh at
HEADERS → BODY transition, but reset closes the defect surface
of ghost state from a previous chunked request. */
void HttpRequestFrontend::reset()
{
    phase_           = ParsePhase::REQUEST_LINE;
    request_         = HttpRequest{};
    body_remaining_  = 0;
    header_bytes_ = 0;
    error_code_      = 0;
    body_chunked_    = false;
    chunk_remaining_ = 0;
    chunk_phase_     = ChunkPhase::SIZE;

    assert(phase_ == ParsePhase::REQUEST_LINE);
    assert(error_code_ == 0);
}

bool HttpRequestFrontend::is_body_in_progress() const
{
    return phase_ == ParsePhase::BODY;
}

size_t HttpRequestFrontend::expected_body_size() const
{
    return request_.body.size() + body_remaining_;
}
