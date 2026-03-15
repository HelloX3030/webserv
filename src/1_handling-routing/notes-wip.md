http/ protocol vs handlers
The handlers are doing too much. Look at HttpMethods_get.cpp:

location matching (Router's job)
method checking (Router's job)
path resolution (could be a utility)
file reading (file serving concern)
response construction (protocol)

These should be thin after Router exists. The question becomes: once Router decides "this is a static file request for /foo/bar.html", who does the file serving?
Options:

handlers/ at top level — handler behaviour is not HTTP protocol knowledge







The handlers currently contain Router logic. After Router exists:

Router: location matching, method dispatch, returns HandlerDecision
Handlers: execute the decision (read file, invoke CGI, build response)

This is the interface contract you need to agree on. Document the HandlerDecision type and who owns what.
