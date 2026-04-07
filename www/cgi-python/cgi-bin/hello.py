#!/usr/bin/env python3

import os

print("Content-Type: text/plain")
print()
print("hello from python CGI")
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
print("QUERY_STRING   =", os.environ.get("QUERY_STRING"))
print("SCRIPT_FILENAME =", os.environ.get("SCRIPT_FILENAME"))
