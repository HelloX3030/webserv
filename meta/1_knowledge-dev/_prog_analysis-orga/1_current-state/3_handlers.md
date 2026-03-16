HttpMethods_get/post/delete analysis
# handlers analysis

HttpMethods_get.cpp, HttpMethods_post.cpp, HttpMethods_delete.cpp


---


## structure of each handler

all 3 follow identical structure:

```
1. location matching        (~15 lines, duplicated)
2. method checking          (~5 lines, duplicated)
3. path resolution          (~10 lines, duplicated)
4. traversal check          (~5 lines, duplicated)
5. method-specific I/O      (unique)
6. response construction    (~5 lines)
```

lines 1-4 are Router concerns, duplicated across all 3 handlers.


---


## duplicated routing logic

extracted pattern (present in all 3):

```cpp
// 1. location matching — longest prefix match
const Location *location = NULL;
std::string location_prefix;

for (auto it = config.locations.begin(); it != config.locations.end(); ++it)
{
    if (path.find(it->first) == 0)
    {
        if (it->first.size() > location_prefix.size())
        {
            location = &it->second;
            location_prefix = it->first;
        }
    }
}

if (!location)
    return HttpResponseBuilder(404);

// 2. method checking
if (location->allowed_methods.count(HttpMethod::GET) == 0)  // varies per handler
    return HttpResponseBuilder(405);

// 3. path resolution
std::string base = location->root;
std::string relative = path.substr(location_prefix.size());
if (!relative.empty() && relative[0] == '/')
    relative = relative.substr(1);

// 4. traversal check
auto safe = utils::resolve_path(base, relative);
if (!safe)
    return HttpResponseBuilder(403);
```

this is config interpretation, not method execution.
belongs in Router, not handlers.


---


## method-specific logic

### GET

```cpp
// directory handling with index files
if (std::filesystem::is_directory(*safe))
{
    for (const auto& idx : location->index_files)
    {
        auto candidate = *safe / idx;
        if (std::filesystem::exists(candidate))
        {
            safe = candidate;
            break;
        }
    }
    // autoindex logic if no index found
}

// file serving
std::ifstream file(safe->c_str(), std::ios::binary);
std::stringstream buffer;
buffer << file.rdbuf();
res.set_body(buffer.str());
```


### POST

```cpp
// body size check (location-level override)
if (location->client_max_body_size.has_value())
    if (content.size() > location->client_max_body_size.value())
        return HttpResponseBuilder(413);

// upload_store handling
std::string base = location->upload_enable ? location->upload_store : location->root;

// file writing
std::ofstream file(safe->c_str(), std::ios::binary);
file.write(content.data(), content.size());

// 200 (overwrite) vs 201 (created)
return HttpResponseBuilder(existed ? 200 : 201);
```


### DELETE

```cpp
// existence + type checks
if (!std::filesystem::exists(*safe))
    return HttpResponseBuilder(404);
if (std::filesystem::is_directory(*safe))
    return HttpResponseBuilder(403);

// deletion
std::filesystem::remove(*safe);
return HttpResponseBuilder(200);
```


---


## what handlers will become (ghr's v2)

after Router exists, handlers receive `HandlerDecision`:

```cpp
HttpResponseBuilder handle_static_file(const HandlerDecision& decision)
{
    // no routing logic — Router already resolved path
    std::ifstream file(decision.file_path, std::ios::binary);
    if (!file.is_open())
        return HttpResponseBuilder(500);

    std::stringstream buffer;
    buffer << file.rdbuf();

    HttpResponseBuilder response(200);
    response.set_body(buffer.str());
    response.set_content_type(decision.file_path);
    return response;
}
```

handler trusts decision. no re-validation. Router is security boundary.


---


## v1 reality

Lukas rejected Router. routing logic stays in handlers.
ghr's HttpRequestFrontend outputs HttpRequest.
dispatch glue (in Connection) calls handlers with (config, uri, body).
handlers do their own routing.

architectural debt. documented for v2.
