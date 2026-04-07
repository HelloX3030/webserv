#!/usr/bin/env python3

import html
from pathlib import Path

BASE_DIR = Path(__file__).resolve().parent.parent
POST_DIR = BASE_DIR / "data" / "posts"
POST_DIR.mkdir(parents=True, exist_ok=True)


def post_title(path):
    try:
        with path.open("r", encoding="utf-8", errors="replace") as handle:
            first_line = handle.readline().strip()
        if first_line.startswith("Title: "):
            return first_line[len("Title: ") :].strip() or path.stem
    except OSError:
        pass
    return path.stem


posts = sorted(
    (entry for entry in POST_DIR.iterdir() if entry.is_file() and entry.suffix == ".txt"),
    key=lambda entry: entry.stat().st_mtime,
    reverse=True,
)

print("Status: 200 OK")
print("Content-Type: text/html; charset=utf-8")
print()
print("<!doctype html>")
print("<html lang='en'>")
print("<head>")
print("<meta charset='UTF-8'>")
print("<meta name='viewport' content='width=device-width, initial-scale=1.0'>")
print("<title>All posts</title>")
print("<style>")
print("body{font-family:system-ui,sans-serif;margin:2rem;line-height:1.5;background:#f6f8fb;color:#1f2937;}")
print("a{color:#2563eb;text-decoration:none;} a:hover{text-decoration:underline;}")
print("ul{list-style:none;padding:0;display:grid;gap:0.75rem;}")
print("li{background:#fff;border:1px solid #d7dfeb;border-radius:0.9rem;padding:1rem;}")
print(".row{display:flex;gap:0.75rem;flex-wrap:wrap;align-items:center;justify-content:space-between;}")
print("form{margin:0;display:inline;}")
print("button{font:inherit;padding:0.45rem 0.75rem;border-radius:0.6rem;border:1px solid #dc2626;background:#dc2626;color:#fff;cursor:pointer;}")
print("</style>")
print("</head>")
print("<body>")
print("<h1>All posts</h1>")
print("<p><a href='/'>Home</a> · <a href='/create.html'>Create post</a> · <a href='/data/posts/'>Raw directory</a></p>")
if posts:
    print("<ul>")
    for post in posts:
        filename = post.name
        title = html.escape(post_title(post))
        safe_filename = html.escape(filename)
        print("<li>")
        print("<div class='row'>")
        print(f"<div><strong>{title}</strong><br><small>{safe_filename}</small></div>")
        print("<div class='row'>")
        print(f"<a href='/cgi-bin/get_post.py?post={safe_filename}'>Open</a>")
        print("<form method='post' action='/cgi-bin/delete_post.py'>")
        print(f"<input type='hidden' name='post' value='{safe_filename}'>")
        print("<button type='submit'>Delete</button>")
        print("</form>")
        print("</div>")
        print("</div>")
        print("</li>")
    print("</ul>")
else:
    print("<p>No posts yet. Create the first one.</p>")
print("</body>")
print("</html>")
