#!/bin/bash

echo "echo CGI"
echo "REQUEST_METHOD=$REQUEST_METHOD"
echo "CONTENT_LENGTH=$CONTENT_LENGTH"

echo "body:"
cat
