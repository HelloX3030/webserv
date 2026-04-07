#!/usr/bin/env python3

import os

print("Status: 200 OK")
print("Content-Type: text/plain; charset=utf-8")
print()
print("cookie inspector")
print("HTTP_COOKIE =", os.environ.get("HTTP_COOKIE", ""))
