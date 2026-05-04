# Config Parser Refactor: Detailed Implementation Checklist

This checklist breaks down the manual refactor of `src/Server.cpp` into concrete, testable steps. Follow in order.

---

## Phase 1: Add Parsing Utilities

### Step 1.1: Add `trim()` helper to Server.cpp (before `find_cgi_path()`)
**What**: Add a simple whitespace trimming utility.
**How**: 
- Insert a new static function above `find_cgi_path()` at line 29 of `src/Server.cpp` that takes a const string reference and returns a trimmed string (spaces and tabs removed from both ends).
- The function should find the first non-whitespace character, then the last non-whitespace character, and return the substring between them. If the string is all whitespace, return empty string.

**Verify**: Compile with `make debug` to ensure no syntax errors.

---

### Step 1.2: Add `tokenizeLine()` helper to Server.cpp (after `trim()`)
**What**: Extract line classification logic into a reusable function.
**How**:
- Define an enum called `LineTokenKind` with four values: `TK_Empty`, `TK_OpenBrace`, `TK_CloseBrace`, and `TK_Directive`.
- Define a struct called `LineToken` with three fields: a `kind` field (of type `LineTokenKind`), a `directive` string (for storing directive names), and a `value` string (for storing directive values).
- Create a function `tokenizeLine()` that takes a const string reference and returns a `LineToken`.
- The function should:
  1. Trim the input line.
  2. If the result is empty, return a token with kind `TK_Empty`.
  3. If the line contains `{`, return `TK_OpenBrace`.
  4. If the line contains `}`, return `TK_CloseBrace`.
  5. If the line contains `=`, extract the part before `=` as the directive name, the part after as the value, trim both, and return a token with kind `TK_Directive` and the extracted directive and value.
  6. Otherwise, return `TK_Empty`.

**Verify**: Compile again. The code should still have no errors.

---

### Step 1.3: Add directive matching helpers (after `tokenizeLine()`)
**What**: Exact-match directive comparison to avoid false positives.
**How**:
- Add three helper functions:
  1. `isDirective()` - takes two const string references and returns true if they are identical. This replaces substring matching.
  2. `isDirectiveRoute()` - takes a directive string and returns true if it equals "route" exactly.
  3. `isDirectiveRouteBlock()` - takes a directive string and returns true if the string contains "route" (this is for catching the route block declaration).

**Why**: This prevents "my_listen" from matching "listen" or "route" from matching "redirection".

---

### Step 1.4: Add parser state enum (after directive matching helpers)
**What**: Define an explicit parser state to replace implicit state tracking via boolean flags.
**How**:
- Define an enum called `ParserState` with three values: `PS_OutsideServer`, `PS_InsideServer`, and `PS_InsideRoute`.
- This makes the parser's current context explicit and enables state-specific error messages.

**Why**: Explicit state is clearer than boolean flags and allows better error reporting (e.g., "unexpected directive while inside route block" vs generic "directive not recognized").

---

## Phase 2: Refactor ImportRoute()

### Step 2.1: Isolate route parsing logic
**What**: Make `ImportRoute()` accumulate fields in a local `Route_rule`, validate completely, then push.
**How**:
- Locate the current route validation block (around line 165 where `routes.push_back(new_route)` happens).
- **Move the `routes.push_back(new_route)` to the end of the function, after all validation.**
- This ensures that if validation fails, the vector is not corrupted.

**Before**:
```cpp
routes.push_back(new_route);  // Line 165 - pushed too early
if (!missing_options.empty())
{
    // validation error
}
```

**After**:
```cpp
if (!missing_options.empty())
{
    // validation error
    throw std::runtime_error("");
}
routes.push_back(new_route);  // Now at the end
```

**Verify**: Recompile. The function should still work but with cleaner semantics.

---

### Step 2.2: Replace substring matching in ImportRoute() with tokenized parsing
**What**: Replace the inline `find("route =")` patterns with tokenized directive matching.
**How**:
1. Locate the parsing loop in `ImportRoute()` that processes each line in `route_raw` (the loop that currently uses `if (str.find("route =") != str.npos)` statements).
2. Replace that entire chain with a loop that:
   - For each line in `route_raw`, call `tokenizeLine()` to get a `LineToken`.
   - Skip any line that is not a `TK_Directive` token.
   - For each directive token, use `isDirective()` to check which directive it is and extract the value:
     - If "route", set `new_route.route` to the token value.
     - If "root", set `new_route.root` to the token value.
     - If "default", set `new_route.default_dir_file` to the token value.
     - If "directorylisting", set `dir_list_present = true` and set `new_route.directorylisting = (token.value == "true")`.
     - If "redirection", set `new_route.redirection` to the token value.
     - If "methods", search the token value for "get", "post", and "delete" substrings and push the corresponding `HttpMethod` enum values.

**Why**: Tokenization normalizes the input and makes matching exact, not substring-based.

**Verify**: Compile and test with `sam.conf` or `djax.conf` to ensure routes parse correctly.

---

### Step 2.3: Add brace validation to prevent malformed routes
**What**: Validate that route block lines do not contain both opening and closing braces or have garbage content after a closing brace.
**How**:
1. In `ImportRoute()`, before collecting lines into `route_raw`, add a check for each line:
   - If a line contains both `{` and `}`, throw an error with the line number (malformed route block).
   - If a line contains `}` and there is non-whitespace content after the `}`, throw an error with the line number.
2. This prevents configs like `route { route = /api }` or `} some_garbage` from being silently parsed as valid.

**Why**: Edge case handling prevents subtle parsing bugs and makes error messages clearer for malformed input.

**Verify**: Test with intentionally malformed route blocks (e.g., `route { directive = value }` on one line) to ensure they are rejected with clear error messages.

---

## Phase 3: Refactor Server::Server()

### Step 3.1: Replace the directive loop with tokenized dispatch
**What**: Refactor the main server constructor to use tokenization instead of the array-index-based switch, with explicit state management.
**How**:
1. Locate the current loop at line 205 that scans the `server_options` array and tries to match directives using a for loop and switch statement.
2. Initialize a `ParserState state = PS_OutsideServer;` variable before the loop begins.
3. Replace that entire section with a single while loop that:
   - Reads a line from the file.
   - Increments the line counter.
   - Calls `tokenizeLine()` to get a token.
   - If the token is `TK_OpenBrace`, check the current state:
     - If state is `PS_OutsideServer`, set state to `PS_InsideServer` and continue.
     - Otherwise, throw an error (unexpected brace).
   - If the token is `TK_CloseBrace`, check the current state:
     - If state is `PS_InsideServer`, set state to `PS_OutsideServer` and break out of the loop.
     - Otherwise, throw an error (unexpected brace in wrong context).
   - If the token is `TK_Empty`, skip it and continue.
   - If the token is `TK_Directive`:
     - Check if state is `PS_InsideServer`; if not, throw an error saying a directive appeared outside a server block, with the line number and directive name.
     - Use `isDirective()` to check the directive name and dispatch to the appropriate handler:
       - "listen" → call `ImportPortPairs()` with the token value.
       - "route" (use `isDirectiveRouteBlock()`) → call `ImportRoute()` with the stream and line counter reference.
       - "403" → set `this->Forbidden` to the token value.
       - "404" → set `this->NotFound` to the token value.
       - "MaxRequestBodySize" → convert the token value to an integer and set `MaxRequestBodySize`.
       - "cgi" → split the token value on the first space to get the file extension and program name, look up the program using `find_cgi_path()`, and if found, add it to the `cgiconfigs` map; if not found, throw an error.
       - Unknown directive → throw an error with the line number and directive name.
4. After the loop exits, check that state is `PS_OutsideServer`; if not, throw an error indicating an unclosed server block.

**Why**: This eliminates the array-index coupling and makes each directive branch explicit and testable.

**Verify**: Compile and parse `djax.conf` and `sam.conf` to ensure all directives still work.

---

### Step 3.2: Simplify value handling in ImportPortPairs()
**What**: Make sure ImportPortPairs() receives a pre-trimmed value.
**How**:
- The function already receives `token.value` which is trimmed by `tokenizeLine()`, so **no changes needed** to the function itself.
- Just verify that it handles the input correctly: `ImportPortPairs(token.value)`.

**Verify**: Test with `listen = 127.0.0.1:8080` configs.

---

## Phase 4: Validation & Edge Cases

### Step 4.1: Test with existing configs
**What**: Ensure the refactored parser produces the same output as the original.
**How**:
1. Compile with `make debug`.
2. Run the server with `./webserv djax.conf`.
3. Verify that:
   - Listen socket is set correctly.
   - All routes are loaded with correct path, root, methods, etc.
   - CGI extensions map to the right programs.
   - Error pages are set.

**Expected**: No crashes, routes listed, server listens on the configured port.

---

### Step 4.2: Test whitespace edge cases
**What**: Confirm trimming is consistent.
**How**: Create a test config with:
```
server {
  listen   =   127.0.0.1:8080
  route   {
    route = /api
  }
  404 =  /errors/404.html
}
```
**Expected**: Parser should handle extra spaces and still extract correct values.

---

### Step 4.3: Test malformed input
**What**: Verify error messages are helpful.
**How**: Test these intentionally broken configs:
1. Missing closing `}` → should error with line number.
2. Unknown directive → should error and reject it.
3. Directive before `{` → should error.
4. Missing space in `cgi = php /usr/bin/php-cgi` → should error.

**Expected**: Each should throw an exception with a clear line number and message.

---

### Step 4.4: Test route block isolation
**What**: Ensure routes do not cross-pollinate state.
**How**: Create a config with multiple route blocks and verify each is independent.
**Expected**: Each route has its own fields, no state leakage.

---

## Phase 5: Cleanup & Documentation

### Step 5.1: Update comments in Server.cpp
**What**: Reflect the new tokenization-based flow.
**How**:
- Update the CONFIG PARSER OVERVIEW comment at the top of the file to mention tokenization.
- Add a brief comment block above `tokenizeLine()` explaining its purpose.

---

### Step 5.2: Final build and test
**What**: Ensure all changes compile and the server still works.
**How**:
1. Run `make clean && make debug`.
2. Fix any remaining warnings or errors.
3. Test with your primary config file one more time.

**Expected**: Clean build, no warnings related to the refactor, server runs.

---

## Rollback / Checkpoint Strategy

- **Before starting**: Save a backup of `src/Server.cpp` to `src/Server.cpp.backup`.
- **After each phase**: Test with a sample config. If it breaks, review the last changes.
- **If stuck**: Compare your version with the backup to identify the divergence.

---

## Summary of Changes

| Component | Old Approach | New Approach |
|-----------|--------------|--------------|
| Line reading | Raw `find()` + `substr()` chains | `tokenizeLine()` returns structured `LineToken` |
| Directive matching | Array index via `switch (i)` | Exact string comparison via `isDirective()` |
| Value extraction | Substring bounds + no trimming guarantee | Tokenizer guarantees trimmed values |
| Error handling | Line numbers + generic messages | Line numbers + directive-specific messages |
| Route validation | Push to vector, then validate | Validate in local struct, then push |
| State tracking | Implicit via loop position | Explicit `ParserState` enum with state transitions |

---

## Expected Outcome

After completing all steps:
- ✅ Parser is resistant to false-positive directive matches ("route" vs "redirection").
- ✅ Values are consistently trimmed.
- ✅ Routes are never partially corrupted on validation failure.
- ✅ Error messages include clear line numbers and directive names.
- ✅ All existing configs parse identically to the original.
- ✅ Malformed input is rejected early with context.