#!/bin/bash

echo "Content-Type: text/plain"
echo

echo "bash cgi env"
echo "REQUEST_METHOD=$REQUEST_METHOD"
echo "QUERY_STRING=$QUERY_STRING"
echo "CONTENT_LENGTH=$CONTENT_LENGTH"

echo
cat
