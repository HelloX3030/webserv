# disk files — multiplexer exemption

## specification

never call `read()`/`recv()` or `write()`/`send()` 
on non-blocking descriptors without prior readiness notification.

exception: regular disk files exempt from multiplexer requirement.

---

## why

`poll()`/`epoll()` on a regular file always returns ready immediately.

the kernel has no "not ready" state for files.
data is either in the page cache or on disk.
either way the kernel fetches it synchronously before returning with no waiting.

sockets and pipes: data arrives asynchronously, driven by remote events.
readiness is genuinely uncertain until the kernel confirms it.

files: you open it, it's there.
`poll()` tells you nothing you didn't already know.

---

## in webserv

serving a static file: `open()` + `read()` directly.
no registration with the multiplexer.

the exemption is not a special case.
it recognises that the readiness problem doesn't exist for files.