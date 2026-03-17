#pragma once

#include "http/HttpRequestFrontend.hpp"

enum class PhaseResult { Advanced, NeedMore, Failed };

// helper declarations — implemented in separate .cpp files
PhaseResult parse_request_line(HttpRequestFrontend& self);
PhaseResult parse_header_line(HttpRequestFrontend& self);
PhaseResult consume_body(HttpRequestFrontend& self);
