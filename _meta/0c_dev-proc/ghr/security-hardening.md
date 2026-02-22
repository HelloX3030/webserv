# security hardening — upcoming

## scope

after core functionality stable:
revisit all input-handling code with adversarial mindset.

## focus areas

### request parser

- no crash on any input
- bounded memory usage per request
- no buffer overflows
- no integer overflows in size calculations
- handle incomplete/truncated input gracefully

### connection handling

- timeout enforcement
- size limit enforcement
- graceful disconnection handling
- no fd leaks on error paths

### CGI

- environment sanitization
- path traversal prevention
- resource limits on child processes

### file serving

- path traversal prevention (no `../` escape from root)
- symlink handling policy
- permission checks

## methodology

1. enumerate all input sources (socket, config, filesystem)
2. for each: list what can go wrong
3. verify code handles each failure mode
4. fuzz test with malformed input
5. stress test with concurrent misbehaving clients