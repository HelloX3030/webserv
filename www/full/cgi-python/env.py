#!/usr/bin/env python3

import os

print("Content-Type: text/plain")
print()
print("CGI env dump (python)")
keys = [
    "REQUEST_METHOD",
    "QUERY_STRING",
    "CONTENT_LENGTH",
    "CONTENT_TYPE",
    "SCRIPT_FILENAME",
    "PATH_INFO"
]
for key in keys:
    print(f"{key}={os.environ.get(key, '')}")
