#!/usr/bin/env python3

import html
import os
import sys
from pathlib import Path
from urllib.parse import parse_qs

BASE_DIR = Path(__file__).resolve().parent.parent
POST_DIR = BASE_DIR / "data" / "posts"
POST_DIR.mkdir(parents=True, exist_ok=True)


def read_fields():
    method = os.environ.get("REQUEST_METHOD", "GET").upper()
    if method == "POST":
        length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
        raw = sys.stdin.read(length) if length > 0 else ""
    else:
        raw = os.environ.get("QUERY_STRING", "")
    return parse_qs(raw, keep_blank_values=True)


fields = read_fields()
filename = fields.get("post", [""])[-1] or fields.get("filename", [""])[-1]
filename = Path(filename).name

def render(message, extra=''):
    print("Status: 200 OK")
    print("Content-Type: text/html; charset=utf-8")
    print()
    print("<!doctype html>")
    print("<html lang='en'>")
    print("<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>")
    print("<title>Delete post</title></head>")
    print("<body style='font-family: system-ui, sans-serif; margin: 2rem; line-height: 1.5;'>")
    print(f"<h1>{message}</h1>")
    if extra:
        print(f"<p>{extra}</p>")
    print("<p><a href='/cgi-bin/list_posts.py'>Back to list</a></p>")
    print("<p><a href='/delete.html'>Delete another</a></p>")
    print("</body></html>")


if not filename:
    render("Missing post filename", "Submit a <code>post</code> field.")
    raise SystemExit(0)

target = POST_DIR / filename
if target.is_file():
    try:
        target.unlink()
        render("Post deleted", f"Removed <strong>{html.escape(filename)}</strong>.")
    except OSError as exc:
        render("Delete failed", html.escape(str(exc)))
else:
    render("Post not found", f"No file named <strong>{html.escape(filename)}</strong> exists.")
