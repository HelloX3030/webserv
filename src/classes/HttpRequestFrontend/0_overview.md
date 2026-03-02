essence, telos, position in system


Key difference from ConfigFrontend: streaming state machine. ConfigFrontend reads entire file then parses. HttpRequestFrontend receives bytes incrementally — must handle "need more data" state.