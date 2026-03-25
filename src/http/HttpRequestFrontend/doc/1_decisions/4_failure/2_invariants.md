# invariants

programming errors are detected by assertions.
a violated invariant means the code is wrong.

upstream: `meta/2_knowledge/failure/0_general/5_detection/1_assertions.md`


---


## the distinction

protocol violations: the input is wrong.
→ expected, return error code, continue serving

invariant violations: the code is wrong.
→ unexpected, abort, fix the defect

the frontend handles protocol violations. it does not handle its own
bugs. if an invariant is violated, the program is in an undefined
state. continuation is meaningless. abort with diagnostic.


---


## invariants in HttpRequestFrontend

properties that must hold if the implementation is correct:


### phase progression

`phase_` moves forward monotonically:
```
REQUEST_LINE → HEADERS → BODY → COMPLETE
                    ↘ ERROR ↙
```

once COMPLETE or ERROR is reached, no further transitions occur
until `reset()`. phase never moves backward during a single parse.

assertion sites:
- `advance()` entry: `phase_` is not COMPLETE (precondition)
- phase transitions: new phase follows legal successor


### error code consistency

`error_code_` is meaningful if and only if `phase_ == ERROR`.

if `phase_ != ERROR`, `error_code_` is uninitialised or stale.
if `phase_ == ERROR`, `error_code_` contains the relevant HTTP code.

assertion sites:
- any transition to ERROR must set `error_code_` first
- returning `Failed` result: `phase_ == ERROR` and `error_code_` is set


### body size bounds

`body_remaining_` never exceeds `max_body_size_`.

the check occurs at HEADERS → BODY transition. if Content-Length
exceeds the limit, transition to ERROR with 413. the body phase
never begins with an oversized value.

assertion site:
- BODY phase entry: `body_remaining_ <= max_body_size_`


### buffer index validity

`find_crlf()` returns a position within `buffer_`. `extract_line()`
and `consume_line()` use this position. indices never exceed
`buffer_.size()`.

assertion sites:
- `extract_line()`: `crlf_pos <= buffer_.size()`
- `consume_line()`: `pos + 2 <= buffer_.size()` (consuming CRLF)


### chunked encoding state

when `body_chunked_ == true`:
- `chunk_phase_` is valid (SIZE, DATA, or TRAILER)
- `chunk_remaining_` is meaningful only in DATA phase

assertion sites:
- chunk phase transitions follow legal successors
- DATA phase: `chunk_remaining_ > 0` or transition occurs


---


## assertion placement

assertions go at boundaries where invariants could be violated:

**function entry (preconditions):**
```cpp
ParseResult HttpRequestFrontend::advance(const char* data, size_t len)
{
    assert(phase_ != ParsePhase::COMPLETE);
    assert(phase_ != ParsePhase::ERROR);
    // ...
}
```

**state transitions:**
```cpp
phase_ = ParsePhase::BODY;
assert(body_remaining_ <= max_body_size_);
```

**function exit (postconditions):**
```cpp
// before returning Failed
assert(phase_ == ParsePhase::ERROR);
assert(error_code_ != 0);
```

the pattern: check at boundaries, where control or data crosses
from one domain to another.


---


## what NOT to assert

**input validity.**
input comes from untrusted clients. checking it is error handling,
not invariant enforcement. use return codes.
```cpp
// wrong: this is input validation
assert(googol == ':');

// right: this is error handling
if (googol != ':')
{
    error_code_ = 400;
    phase_ = ParsePhase::ERROR;
    return PhaseResult::Failed;
}
```

**recoverable conditions.**
if the condition can arise in correct operation and has a defined
response, it is not an invariant.
```cpp
// wrong: incomplete input is expected
assert(googol_crlf(pos));

// right: return Incomplete
if (!find_crlf(pos))
    return PhaseResult::NeedMore;
```

**expensive checks.**
assertions should be cheap relative to the operation. if verification
is expensive, consider debug-only checks or separate validation passes.


---


## debug vs release

C++ `assert()` is disabled when `NDEBUG` is defined.

options:

**disable in release (default):**
- performance: zero cost when disabled
- risk: bugs that escape testing cause undefined behaviour silently

**keep in release:**
- safety: bugs cause immediate abort with diagnostic
- cost: performance overhead (usually negligible for these checks)
- exposure: assertion messages may leak internal details

**custom macro:**
- `WEBSERV_ASSERT`: always active, but with opaque production messages
- debug: full diagnostic (file, line, condition)
- release: minimal message, no internal details

the adversarial concern: an assertion that fires in production is a
DoS vector. but silent continuation past a violated invariant may be
worse — memory corruption, authentication bypass, data loss.

for WebServ: assertions remain active. the checks are cheap. the
alternative (undefined behaviour) is unacceptable. production messages
are kept minimal.


---


## deriving invariants from code

invariants are not invented — they are discovered by examining what
must be true for the code to function correctly.

method:
1. identify state variables: `phase_`, `body_remaining_`, `buffer_`, etc.
2. for each variable, ask: what values are valid? when?
3. for each function, ask: what must be true on entry? on exit?
4. for each transition, ask: what relationship must hold before/after?

the answers are invariants. encode them as assertions.

if an invariant is difficult to state precisely, the design may be
unclear. clarifying invariants often reveals simplifications.


---


## summary

invariants are properties that must hold if the code is correct.
violation indicates a bug, not bad input.

detection: assertions. response: abort.

key invariants in HttpRequestFrontend:
- phase progression is monotonic
- error code is set iff phase is ERROR
- body remaining never exceeds max size
- buffer indices stay within bounds
- chunked state is consistent

assert at boundaries: function entry, state transitions, function exit.
do not assert input validity or recoverable conditions.
