#!/usr/bin/env python3

import os
from urllib.parse import parse_qs

query = parse_qs(os.environ.get("QUERY_STRING", ""))
name = query.get("name", ["world"])[0]

print("Content-Type: text/plain")
print()
print("hello from full/python CGI")
print("name =", name)
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
