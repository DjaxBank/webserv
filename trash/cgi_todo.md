# CGI Implementation Checklist

## 1. Config Parser (`Server.cpp` / `Route_rule`)

### Add to `Route_rule` struct:
```cpp
bool        cgi_enabled;
std::string cgi_bin_path;
std::map<std::string, std::string> cgi_extensions; // ".py" -> "/usr/bin/python3"
int         cgi_timeout; // seconds, optional but recommended
```

### Add to config file format:
```
route {
    route = /cgi-bin
    root = /var/www/cgi-bin
    cgi = true
    cgi_extension = .py /usr/bin/python3
    cgi_extension = .php /usr/bin/php
    cgi_timeout = 30
    default = index.html
    directorylisting = false
}
```

### Parsing logic needed:
- Parse `cgi = true/false` flag
- Parse `cgi_extension` as repeatable key (loop, not single find) — split on space into ext + interpreter
- Parse `cgi_timeout` as integer

### Existing bugs to fix:
- `ImportRoute`: `bool open, closed` are uninitialized — UB
- `Server` constructor: `bool open` is uninitialized — UB

```cpp
// fix both to:
bool open = false, closed = false, dir_list_present = false;
```

---

## 2. Request Parser

### Bugs to fix:
- `requestURIParsing.cpp` — `parseURI()` hardcodes the URI instead of using the actual request:
```cpp
// current (wrong):
std::string uri = "/a/./b";

// fix:
std::string uri = m_request.getRawUri();
```

### Additions needed:
- Split normalized path into `SCRIPT_NAME` and `PATH_INFO`:
  - `SCRIPT_NAME` = path up to and including the script file (e.g. `/cgi-bin/hello.py`)
  - `PATH_INFO` = everything after the script file (e.g. `/extra/path`)
  - Detection: walk the path, check if a segment corresponds to an actual file on disk

---

## 3. CGI Executor (new component — bulk of the work)

### Environment variables to build:

From the request parser (already available):
```
REQUEST_METHOD    request.getMethod()
QUERY_STRING      request.getQuery()
CONTENT_LENGTH    request.getContentLen()
CONTENT_TYPE      request.getHeader("content-type")
SCRIPT_NAME       script_name (split from path)
PATH_INFO         path_info (split from path)
SERVER_PROTOCOL   version_tostring(request.getVersion())
```

From server/socket (already available):
```
SERVER_NAME       sock.interface
SERVER_PORT       sock.port
```

Static strings:
```
GATEWAY_INTERFACE=CGI/1.1
```

From headers map (iterate and transform):
```
// lowercase "content-type" -> "HTTP_CONTENT_TYPE"
// replace '-' with '_', uppercase, prefix with "HTTP_"
```

From connection layer (not yet available — needs threading through from accept()):
```
REMOTE_ADDR       client IP from accept()
```

### fork/exec skeleton:
```cpp
int stdin_pipe[2];
int stdout_pipe[2];
pipe(stdin_pipe);
pipe(stdout_pipe);

pid_t pid = fork();
if (pid == 0)
{
    dup2(stdin_pipe[0], STDIN_FILENO);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    close(stdin_pipe[1]);
    close(stdout_pipe[0]);

    char* argv[] = { (char*)interpreter.c_str(), (char*)script_path.c_str(), nullptr };
    execve(interpreter.c_str(), argv, envp);
    exit(1); // execve failed
}
// parent:
// 1. write POST body to stdin_pipe[1]
// 2. close stdin_pipe[1] so child sees EOF
// 3. read response from stdout_pipe[0]
// 4. wait for child / enforce timeout
```

### Timeout handling:
- After fork, track elapsed time
- If child hasn't exited within `cgi_timeout` seconds, `kill(pid, SIGKILL)`
- Return 504 Gateway Timeout to client

### CGI response parsing:
- Script stdout = headers, then blank line (`\r\n\r\n`), then body
- Parse headers from stdout the same way you parse HTTP headers
- Handle `Status:` header specially — it is not a real HTTP header, translate it into the HTTP response status line:
  ```
  Status: 404 Not Found  ->  HTTP/1.1 404 Not Found
  ```
- If no `Status:` header present, default to `200 OK`
- Forward remaining headers and body to client as normal HTTP response

### Error cases to handle:
- `execve` fails (wrong interpreter path, bad permissions) → 500
- Script exits non-zero → 500, log stderr separately
- Script exceeds timeout → 504, kill child
- Script produces no output → 500
- `pipe()` or `fork()` fails → 500

---

## 4. Summary of Effort

| Area | Work |
|---|---|
| Config parser additions | Small (~20-30 lines) |
| Request parser URI fix | Trivial (1 line) |
| SCRIPT_NAME / PATH_INFO split | Small (~20 lines) |
| REMOTE_ADDR threading | Small (pass client IP from accept()) |
| CGI executor | Large (~150-200 lines) |
