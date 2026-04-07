#!/usr/bin/env python3

import html
import os
from pathlib import Path
from urllib.parse import parse_qs

BASE_DIR = Path(__file__).resolve().parent.parent
POST_DIR = BASE_DIR / "data" / "posts"
POST_DIR.mkdir(parents=True, exist_ok=True)


def selected_post():
    query = parse_qs(os.environ.get("QUERY_STRING", ""), keep_blank_values=True)
    value = query.get("post", [""])[-1] or query.get("filename", [""])[-1]
    return Path(value).name


filename = selected_post()
target = POST_DIR / filename

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print()
print("<!doctype html>")
print("<html lang='en'>")
print("<head>")
print("<meta charset='UTF-8'>")
print("<meta name='viewport' content='width=device-width, initial-scale=1.0'>")
print(f"<title>{html.escape(filename or 'Post')}</title>")
print("<style>")
print("body{font-family:system-ui,sans-serif;margin:2rem;line-height:1.5;background:#f6f8fb;color:#1f2937;}")
print("a{color:#2563eb;text-decoration:none;} a:hover{text-decoration:underline;}")
print("pre{white-space:pre-wrap;word-break:break-word;background:#fff;border:1px solid #d7dfeb;border-radius:0.9rem;padding:1rem;}")
print(".meta{color:#6b7280;}")
print("</style>")
print("</head>")
print("<body>")
print("<p><a href='/cgi-bin/list_posts.py'>Back to list</a> · <a href='/create.html'>Create post</a></p>")
if not filename:
    print("<h1>Missing post parameter</h1>")
    print("<p>Use <code>?post=filename.txt</code>.</p>")
else:
    if target.is_file():
        try:
            text = target.read_text(encoding='utf-8', errors='replace')
        except OSError:
            text = ''
        lines = text.splitlines()
        title = filename
        created = ''
        stored_name = filename
        body_start = 0
        if lines and lines[0].startswith('Title: '):
            title = lines[0][7:].strip() or filename
            body_start = 1
        if len(lines) > 1 and lines[1].startswith('Created: '):
            created = lines[1][9:].strip()
            body_start = 2
        if len(lines) > 2 and lines[2].startswith('Filename: '):
            stored_name = lines[2][10:].strip() or filename
            body_start = 4 if len(lines) > 3 and lines[3] == '' else 3
        body = "\n".join(lines[body_start:]).strip()
        print(f"<h1>{html.escape(title)}</h1>")
        if created or stored_name != filename:
            print("<p class='meta'>")
            if created:
                print(f"Created: {html.escape(created)}<br>")
            print(f"Filename: {html.escape(stored_name)}")
            print("</p>")
        print(f"<pre>{html.escape(body or '(empty post)')}</pre>")
    else:
        print("<h1>Post not found</h1>")
        print(f"<p>No file named <strong>{html.escape(filename)}</strong> exists.</p>")
print("</body>")
print("</html>")
