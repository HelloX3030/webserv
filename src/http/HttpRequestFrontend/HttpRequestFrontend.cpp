#include "http/HttpRequestFrontend.hpp"
#include "HttpRequestFrontend_internal.hpp"

/* construction.
max_body_size from server configuration — constant for connection lifetime.
all other fields initialised to their pre-parse state.
see state machine doc invariants 1–8. */
HttpRequestFrontend::HttpRequestFrontend(size_t max_body_size)
    : buffer_()
    , phase_(ParsePhase::REQUEST_LINE)
    , request_()
    , body_remaining_(0)
    , error_code_(0)
    , max_body_size_(max_body_size)
    , body_chunked_(false)
    , chunk_remaining_(0)
    , chunk_phase_(ChunkPhase::SIZE)
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
                    return {ParseStatus::Failed, {}, error_code_};
                break;
            }

            case ParsePhase::HEADERS:
            {
                PhaseResult r = parse_header_line();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                    return {ParseStatus::Failed, {}, error_code_};
                break;
            }

            case ParsePhase::BODY:
            {
                PhaseResult r = consume_body();
                if (r == PhaseResult::NeedMore)
                    return {ParseStatus::Incomplete, {}, 0};
                if (r == PhaseResult::Failed)
                    return {ParseStatus::Failed, {}, error_code_};
                break;
            }

            case ParsePhase::COMPLETE:
                return {ParseStatus::Complete, request_, 0};

            case ParsePhase::ERROR:
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
    error_code_      = 0;
    body_chunked_    = false;
    chunk_remaining_ = 0;
    chunk_phase_     = ChunkPhase::SIZE;
}
