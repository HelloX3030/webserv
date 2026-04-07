#!/usr/bin/env python3

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print("Set-Cookie: ws_session=abc123; Path=/; Max-Age=3600; HttpOnly")
print("Set-Cookie: ws_theme=dark; Path=/; Max-Age=3600")
print()
print("<!doctype html>")
print("<html><body>")
print("<h1>Cookies set</h1>")
print("<p>Set cookies: ws_session, ws_theme</p>")
print("<p><a href='/cgi-bin/show-cookies.py'>Now show cookies</a></p>")
print("</body></html>")
