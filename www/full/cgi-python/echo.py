#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/plain")
print()
print("echo from full/python CGI")
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
print("CONTENT_LENGTH =", os.environ.get("CONTENT_LENGTH"))
print()
print(sys.stdin.read())
