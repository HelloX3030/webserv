# Webserv architecture documentation

Hey Lukas,
check out these docs in this order:

## 0. Component architecture
Defines what components exist, what they contain, and how they interact.
**Read this first** - establishes the pieces we're building.

## 1. Namespacing
Explains code organization pattern and fixes current encapsulation issues.
**Read this second** - shows how to organize the pieces.

## 2. Event loop
Defines where event loop lives and fixes current multiple-loops issue.
**Read this third** - establishes control flow.

## 3. Poll infrastructure  
Justifies poll() over epoll() for I/O multiplexing.
**Read this last** - implementation technology choice.