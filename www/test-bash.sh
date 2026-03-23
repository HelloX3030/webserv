#!/bin/bash

echo "Content-Type: text/plain"
echo

echo "Hello from Bash CGI"
echo
echo "REQUEST_METHOD = $REQUEST_METHOD"
echo "QUERY_STRING   = $QUERY_STRING"

if read body; then
    echo
    echo "BODY:"
    echo "$body"
fi
