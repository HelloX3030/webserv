#include "http/HttpRequest.hpp"

bool HttpRequest::keepAlive() const { return false; }
long HttpRequest::contentLength() const { return -1; }
