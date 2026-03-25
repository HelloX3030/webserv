On HttpRequestFrontend failure docs:

Current 0_categories.md opens with "three categories of failure, distinguished by cause and response."

The problem: this conflates multiple dimensions and misses the upstream grounding.

From the meta discussion into failure -> ontology: failure is contract deviation.

The question for HttpRequestFrontend is: what is this component's contract, and how can it be violated?

The contract:
advance : bytes → ParseResult
ParseResult = Complete HttpRequest | Incomplete | Failed ErrorCode

Given bytes, produce a parse result. No I/O. Pure transformation.


Violations of this contract:

Input violates protocol — the bytes do not constitute valid HTTP. This is expected. Clients are untrusted.
The contract explicitly handles this: return Failed ErrorCode.
This is not "failure" of the component — it is correct operation. The component successfully detected invalid input.

Code violates its own invariants — a bug. phase_ reaches an impossible state. Buffer index exceeds bounds.
This should never happen if the code is correct.
Detection: assertions. Response: abort. These are defects, not runtime conditions.

System failures (I/O errors) never reach the frontend. Connection owns that boundary. The frontend's input is bytes already read.
