#!/usr/bin/env python3

import html
import os
import re
import sys
from datetime import datetime
from pathlib import Path
from urllib.parse import parse_qs

BASE_DIR = Path(__file__).resolve().parent.parent
POST_DIR = BASE_DIR / "data" / "posts"
POST_DIR.mkdir(parents=True, exist_ok=True)


METHOD = os.environ.get("REQUEST_METHOD", "GET").upper()


def read_fields():
    if METHOD == "POST":
        length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
        raw = sys.stdin.read(length) if length > 0 else ""
    else:
        raw = ""
    values = parse_qs(raw, keep_blank_values=True)
    return {key: values.get(key, [""])[-1] for key in values}


def slugify(value):
    value = value.strip().lower()
    value = re.sub(r"[^a-z0-9]+", "-", value)
    value = re.sub(r"-+", "-", value).strip("-")
    return value or "post"


fields = read_fields()
if METHOD != "POST":
    print("Status: 200 OK")
    print("Content-Type: text/html; charset=utf-8")
    print()
    print("<!doctype html>")
    print("<html lang='en'>")
    print("<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>")
    print("<title>Create post</title></head>")
    print("<body style='font-family: system-ui, sans-serif; margin: 2rem; line-height: 1.5;'>")
    print("<h1>Create post</h1>")
    print("<p>Use the HTML form at <a href='/create.html'>/create.html</a> to submit a post with POST.</p>")
    print("</body></html>")
    raise SystemExit(0)

title = fields.get("title", "").strip() or "Untitled post"
content = fields.get("content", "")
slug = slugify(title)
timestamp = datetime.now().strftime("%Y%m%d%H%M%S")
filename = f"{timestamp}_{slug}.txt"
path = POST_DIR / filename
suffix = 1
while path.exists():
    filename = f"{timestamp}_{slug}-{suffix}.txt"
    path = POST_DIR / filename
    suffix += 1

created_at = datetime.now().isoformat(timespec='seconds')
with path.open("w", encoding="utf-8") as handle:
    handle.write(f"Title: {title}\n")
    handle.write(f"Created: {created_at}\n")
    handle.write(f"Filename: {filename}\n\n")
    handle.write(content)
    if content and not content.endswith("\n"):
        handle.write("\n")

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print()
print("<!doctype html>")
print("<html lang='en'>")
print("<head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>")
print("<title>Post created</title>")
print("</head>")
print("<body style='font-family: system-ui, sans-serif; margin: 2rem; line-height: 1.5;'>")
print("<h1>Post created</h1>")
print(f"<p>Saved as <strong>{html.escape(filename)}</strong>.</p>")
print(f"<p><a href='/cgi-bin/get_post.py?post={html.escape(filename)}'>Open the post</a></p>")
print("<p><a href='/cgi-bin/list_posts.py'>Back to the list</a></p>")
print("<p><a href='/create.html'>Create another post</a></p>")
print("</body></html>")
