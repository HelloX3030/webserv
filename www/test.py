#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/plain")
print()

print("Hello from CGI!")
print("")

print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
print("QUERY_STRING   =", os.environ.get("QUERY_STRING"))

body = sys.stdin.read()
if body:
    print("")
    print("BODY:")
    print(body)
