# stat - get file status information
query file entity properties

## signature

```cpp
#include <sys/stat.h>

int stat(const char *pathname, struct stat *statbuf);
```

## semantics

role: resolve pathname to inode, then extract metadata about the file
entity without opening it.

returns: 0 on success, -1 on error (sets `errno`)

the `struct stat` contains:
- `st_mode` - file type and permissions
- `st_size` - file size in bytes
- `st_mtime` - last modification time
- `st_ino` - inode number
- `st_dev` - device ID
- `st_nlink` - number of hard links
- `st_uid`, `st_gid` - owner, group
- additional timestamps: `st_atime`, `st_ctime`

key insight: `stat` performs name resolution but does not create a file
descriptor. it's a read-only namespace query.

## edge cases

symbolic links: `stat` follows symlinks (resolves to target). use
`lstat` to query the link itself.

nonexistent paths: returns -1, `errno = ENOENT`

permission denied: returns -1, `errno = EACCES` (cannot traverse
directory in path)

path too long: `errno = ENAMETOOLONG`

race conditions: file can change/disappear between `stat` and subsequent
operations. not atomic with `open`.

## webserv relevance

critical for http servers:

1. determine file type: is request path a regular file or directory?
   - check `S_ISREG(statbuf.st_mode)` for files
   - check `S_ISDIR(statbuf.st_mode)` for directories

2. content-length header: `st_size` gives exact file size for response

3. last-modified header: `st_mtime` for caching/conditional requests

4. security: check permissions before attempting to serve file

5. directory listings: verify path is directory before calling `opendir`

typical webserv usage:
```cpp
struct stat file_stat;
if (stat(requested_path.c_str(), &file_stat) == -1) {
    // file not found → 404 response
}
if (S_ISDIR(file_stat.st_mode)) {
    // try index.html or generate directory listing
}
// use file_stat.st_size for content-length
```

## questions

why not just try to open and handle errors?
- `stat` is cheaper than `open` (no fd allocation)
- need metadata before deciding whether to open
- can check multiple paths without exhausting file descriptors