#!/bin/bash

# Note: current webserv CGI integration returns stdout as response body.
# Keep output plain text so the result is easy to read.

echo "hello from bash CGI"
echo "REQUEST_METHOD=$REQUEST_METHOD"
echo "QUERY_STRING=$QUERY_STRING"
