- Identified routing logic duplication in HttpMethods_*.cpp
- Proposed Router interface: (HttpRequest, ServerConfig) → HandlerDecision
- Proposed handler interface: HandlerDecision → HttpResponse
- Identified HttpResponseFrontend purity violation
- Corrected interface: serialize(HttpResponse) → bytes (pure)
- Clarified: Runtime owns I/O, frontends own transformation


UPCOMING
- Full mental model of request lifecycle
- www/ integration with config.error_pages
- Router implementation
- HttpResponseFrontend implementation
- Lukas's handler refactor (remove routing logic)
