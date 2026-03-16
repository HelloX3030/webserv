# error semantics

## parse failure → connection close

parse errors leave the byte stream in an indeterminate state.
without knowing where the current (malformed) request ends,
the frontend cannot find the start of the next request.

therefore: parse failure implies connection close.

the frontend reports failure. the runtime owns the lifecycle decision.
```
ParseResult.status == Failed
    → runtime generates error response
    → runtime writes to fd
    → runtime closes connection
```

the frontend does not close. it does not know about fds.
it returns (Failed, error_code). runtime acts.


## error codes

| code | meaning                        |
|------|--------------------------------|
| 400  | malformed request line/header  |
| 413  | body exceeds limit             |
| 501  | unknown method                 |
