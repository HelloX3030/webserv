#!/usr/bin/env python3

import os
import sys

print("Content-Type: text/plain")
print()

print("========== BASIC ==========")
print("REQUEST_METHOD =", os.environ.get("REQUEST_METHOD"))
print("QUERY_STRING   =", os.environ.get("QUERY_STRING"))
print("CONTENT_LENGTH =", os.environ.get("CONTENT_LENGTH"))
print("CONTENT_TYPE   =", os.environ.get("CONTENT_TYPE"))

print()
print("========== SCRIPT ==========")
print("SCRIPT_FILENAME =", os.environ.get("SCRIPT_FILENAME"))
print("SCRIPT_NAME     =", os.environ.get("SCRIPT_NAME"))
print("PATH_INFO       =", os.environ.get("PATH_INFO"))

print()
print("========== SERVER ==========")
print("SERVER_PROTOCOL   =", os.environ.get("SERVER_PROTOCOL"))
print("GATEWAY_INTERFACE =", os.environ.get("GATEWAY_INTERFACE"))
print("SERVER_NAME       =", os.environ.get("SERVER_NAME"))
print("SERVER_PORT       =", os.environ.get("SERVER_PORT"))

print()
print("========== HEADERS ==========")
print("HTTP_HOST       =", os.environ.get("HTTP_HOST"))
print("HTTP_USER_AGENT =", os.environ.get("HTTP_USER_AGENT"))
print("HTTP_ACCEPT     =", os.environ.get("HTTP_ACCEPT"))
print("HTTP_COOKIE     =", os.environ.get("HTTP_COOKIE"))

print()
print("========== BODY ==========")

body = sys.stdin.read()
if body:
    print(body)

print()
print("========== FULL ENV ==========")
for key in sorted(os.environ.keys()):
    print(f"{key}={os.environ[key]}")
