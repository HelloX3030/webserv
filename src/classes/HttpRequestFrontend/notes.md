ctx, location in sys

                    ┌─────────────────────┐
                    │    Config Frontend  │  ← startup phase
                    └──────────┬──────────┘
                               │ produces
                               ▼
                    ┌─────────────────────┐
                    │   ServerConfig[]    │  ← pure data
                    └──────────┬──────────┘
                               │ used by
                               ▼
┌───────────────────────────────────────────────────────────────┐
│                        Event Loop                             │
│  ┌────────────┐     ┌───────────────────────────────────── ─┐ │
│  │   poll()   │ ──► │         Connection (per-client)       │ │
│  └────────────┘     │  ┌───────────────────────────────────┐│ │
│                     │  │      HTTP Request Front end       ││ │
│                     │  │  (state machine inside Connection)││ │
│                     │  └───────────────────────────────────┘│ │
│                     └───────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────┘


potential files:

1_cursor    // buffer pos. peek / consume (shame can't slurp)
2_request_line // Method SP URI SP Version CRLF
3_headers
4_body