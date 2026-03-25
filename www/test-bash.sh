#!/bin/bash

echo "Content-Type: text/plain"
echo

echo "========== BASIC =========="
echo "REQUEST_METHOD = $REQUEST_METHOD"
echo "QUERY_STRING   = $QUERY_STRING"
echo "CONTENT_LENGTH = $CONTENT_LENGTH"
echo "CONTENT_TYPE   = $CONTENT_TYPE"

echo
echo "========== SCRIPT =========="
echo "SCRIPT_FILENAME = $SCRIPT_FILENAME"
echo "SCRIPT_NAME     = $SCRIPT_NAME"
echo "PATH_INFO       = $PATH_INFO"

echo
echo "========== SERVER =========="
echo "SERVER_PROTOCOL = $SERVER_PROTOCOL"
echo "GATEWAY_INTERFACE = $GATEWAY_INTERFACE"
echo "SERVER_NAME     = $SERVER_NAME"
echo "SERVER_PORT     = $SERVER_PORT"

echo
echo "========== HEADERS =========="
echo "HTTP_HOST       = $HTTP_HOST"
echo "HTTP_USER_AGENT = $HTTP_USER_AGENT"
echo "HTTP_ACCEPT     = $HTTP_ACCEPT"
echo "HTTP_COOKIE     = $HTTP_COOKIE"

echo
echo "========== BODY =========="
if read body; then
    echo "$body"
fi
