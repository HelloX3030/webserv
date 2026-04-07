#!/usr/bin/env python3

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print("Set-Cookie: blog_session=xyz789; Path=/; Max-Age=3600; HttpOnly")
print("Set-Cookie: blog_pref=light; Path=/; Max-Age=3600")
print()
print("<html><body><h1>Cookies set</h1><p><a href='/cookies-bin/show-cookies.py'>Show cookies</a></p></body></html>")
