#!/usr/bin/env python3

import os
import sys

print("Status: 200 OK")
print("Content-Type: text/plain; charset=utf-8")
print()
print("python cgi env")
print("REQUEST_METHOD=", os.environ.get("REQUEST_METHOD"))
print("QUERY_STRING=", os.environ.get("QUERY_STRING"))
print("CONTENT_LENGTH=", os.environ.get("CONTENT_LENGTH"))

body = sys.stdin.read()
if body:
    print("BODY=")
    print(body)
