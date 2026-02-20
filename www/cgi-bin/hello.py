#!/usr/bin/env python3

import os

method = os.environ.get("REQUEST_METHOD", "unknown")
query  = os.environ.get("QUERY_STRING", "")

print("Content-Type: text/plain\r")
print("\r")
print("hello from CGI")
print("method: " + method)
if query:
    print("query: " + query)