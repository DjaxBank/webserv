# URI Parsing Guide for Webserver Project

## What You Actually Need vs The Full Spec

The RFC 3986 specification covers **all possible URI uses everywhere**. Your webserver only needs to handle the **subset that appears in HTTP request lines**, which is much simpler.

---

## Table of Contents
1. [What You Actually Receive](#what-you-actually-receive)
2. [Essential Components](#essential-components)
3. [Simplified Parser Flow](#simplified-parser-flow)
4. [Must Have Features](#must-have-features)
5. [Should Have Features](#should-have-features)
6. [Don't Need Features](#dont-need-features)
7. [Security Essentials](#security-essentials)
8. [Quick Reference](#quick-reference)

---

## What You Actually Receive

### **Typical HTTP Request**
```
GET /path/to/file?key=value HTTP/1.1
Host: example.com
```

### **What's in the Request Line**
- **Method**: GET, POST, DELETE, etc.
- **Request-URI**: `/path/to/file?key=value`
- **Version**: HTTP/1.1

**Note**: The scheme (`http://`) and authority (`example.com`) are typically NOT in the request line - they're in the `Host:` header instead.

### **Proxy Request (Less Common)**
```
GET http://example.com/path?query HTTP/1.1
```

Only appears when your server acts as a proxy. Most basic webserv projects don't implement this.

---

## Essential Components

### **What You Need to Parse from Request-URI**

#### **1. Path** - ALWAYS PRESENT (can be empty)
```
/path/to/file
```
- Starts with `/` in normal requests
- Can be empty (which normalizes to `/`)
- Contains the resource location

#### **2. Query** - OPTIONAL
```
?key=value&foo=bar
```
- Everything after the `?`
- Used for CGI parameters, search queries, etc.
- Can be absent entirely

#### **3. Fragment** - IGNORE
```
#section
```
- Clients don't send fragments to servers
- If present, strip it or ignore it
- Not used in server-side processing

### **What You DON'T Need to Parse**

For standard webserver requests:
- ❌ Scheme (`http://`, `https://`)
- ❌ Authority (`user@host:port`)
- ❌ Userinfo (`user:password@`)
- ❌ Host (it's in the `Host:` header)
- ❌ Port (it's in the `Host:` header or server config)

---

## Simplified Parser Flow

### **Step 1: Extract Request-URI from Request Line**

Input: `GET /path?query HTTP/1.1`

- Split on spaces
- First part = method
- Middle part = request-URI
- Last part = version

### **Step 2: Handle Fragment (if present)**

If request-URI contains `#`:
- Strip everything from `#` onwards
- Or reject the request (fragments shouldn't be sent to servers)

Example: `/path?query#fragment` → `/path?query`

### **Step 3: Split Path and Query**

If request-URI contains `?`:
- Everything before `?` = path
- Everything after `?` = query

Examples:
- `/path?query` → path: `/path`, query: `query`
- `/path` → path: `/path`, query: undefined
- `/path?` → path: `/path`, query: empty string

### **Step 4: Percent-Decode the Path**

**IMPORTANT**: Decode AFTER splitting components, not before.

Convert percent-encodings to actual characters:
- `%20` → space
- `%2F` → `/`
- `%41` → `A`

**Watch for dangerous encodings**:
- `%00` → null byte (REJECT this)
- `%2E` → `.` (could bypass dot-segment removal if decoded too early)
- `%2F` → `/` (could bypass path splitting if decoded too early)

### **Step 5: Remove Dot-Segments (CRITICAL for security)**

Process special path segments:

**Current directory** (`.`):
- `/a/./b` → `/a/b`
- Just remove it

**Parent directory** (`..`):
- `/a/b/../c` → `/a/c`
- Go up one level
- **Never go above root**: `/../etc/passwd` → `/etc/passwd`

### **Step 6: Validate**

Check for security issues:
- Path must start with `/`
- No null bytes anywhere
- Reasonable length (typically < 8KB)
- Valid UTF-8 after decoding
- Path doesn't escape document root

### **Step 7: Done**

You now have:
- Parsed path (decoded and normalized)
- Query string (if present)
- Ready to use for file serving, CGI, routing, etc.

---

## Must Have Features

### **1. Path Parsing**
Extract the path portion from the request-URI.

### **2. Dot-Segment Removal**
Remove `.` and `..` from paths to prevent directory traversal attacks.

**Why this matters**:
```
Request: GET /../../../etc/passwd HTTP/1.1
Without removal: tries to access /etc/passwd
With removal: normalized to /etc/passwd, which you can then check against document root
```

### **3. Percent-Decoding**
Convert `%XX` encodings to actual characters.

**Why this matters**:
```
Request: GET /my%20file.html HTTP/1.1
Must decode to: /my file.html
Otherwise file system can't find it
```

### **4. Null Byte Rejection**
Reject any request containing `%00`.

**Why this matters**:
```
Request: GET /index.html%00.jpg HTTP/1.1
Some systems treat this as /index.html (null terminates string)
But attacker bypassed your ".jpg only" filter
```

### **5. Basic Validation**
- Reject malformed requests
- Reject overly long URIs
- Reject invalid percent-encodings (`%ZZ`, `%1G`)

---

## Should Have Features

### **1. Query String Parsing**
Split on `?` and extract query parameters.

Needed for:
- CGI scripts
- Dynamic content
- URL parameters

### **2. Fragment Handling**
Strip `#` and everything after it, or reject requests with fragments.

### **3. Path Normalization**
- Convert to lowercase (if your filesystem is case-insensitive)
- Remove duplicate slashes: `//path///file` → `/path/file`
- Ensure paths start with `/`

### **4. Length Limits**
Reject URIs exceeding reasonable limits:
- Total URI length: 2KB - 8KB typical
- Path length: 255 characters typical
- Query length: depends on your needs

Prevents DoS attacks with extremely long URIs.

### **5. Character Validation**
Reject or encode dangerous characters in paths:
- Null bytes
- Control characters
- OS-specific reserved characters

---

## Don't Need Features

These are in the full RFC but NOT needed for basic webserver:

### **1. Scheme Parsing**
No need to parse `http://` or `https://` - not in typical requests.

### **2. Authority Parsing**
No need to parse `user@host:port` from the URI - use `Host:` header instead.

### **3. Userinfo Handling**
No need to handle `username:password@` - it's deprecated and insecure.

### **4. Port Extraction**
No need to parse port from URI - it's in `Host:` header or server configuration.

### **5. IPv6 Literal Support**
No need to parse `[::1]` style addresses from URI.

### **6. Complex Relative Reference Resolution**
No need for the full RFC algorithm - you only deal with absolute paths.

### **7. URI Comparison/Equivalence**
Basic string matching after normalization is sufficient.

### **8. Protocol-Based Normalization**
Scheme-specific rules not needed for basic HTTP serving.

---

## Security Essentials

### **1. Directory Traversal Prevention**

**Attack**:
```
GET /../../../etc/passwd HTTP/1.1
```

**Defense**:
- Remove all dot-segments
- Never let normalized path go above document root
- Check final path is within allowed directory

### **2. Null Byte Injection**

**Attack**:
```
GET /allowed.jpg%00../../etc/passwd HTTP/1.1
```

**Defense**:
- Reject any URI containing `%00`
- Check for null bytes after decoding

### **3. Double Encoding**

**Attack**:
```
GET /%252e%252e/etc/passwd HTTP/1.1
(%25 = %, so %252e = %2e = .)
```

**Defense**:
- Decode only once
- Track if already decoded
- Reject suspicious patterns

### **4. Path Separator Encoding**

**Attack**:
```
GET /safe%2f..%2f..%2fetc%2fpasswd HTTP/1.1
(%2f = /)
```

**Defense**:
- Decode percent-encodings BEFORE removing dot-segments
- But AFTER splitting on delimiters

### **5. Special File System Characters**

**Attack**:
```
GET /file:stream HTTP/1.1  (Windows alternate data stream)
GET /aux HTTP/1.1          (Windows reserved name)
```

**Defense**:
- Validate against OS-specific reserved names
- Reject or sanitize special characters: `<>:"|?*\`

### **6. Extremely Long URIs**

**Attack**:
```
GET /aaaa...aaaa (megabytes of 'a's)
```

**Defense**:
- Limit URI length (2KB - 8KB typical)
- Reject before allocating large buffers

---

## Percent-Encoding Reference

### **How It Works**

Format: `%` followed by two hexadecimal digits

Examples:
- `%20` = space
- `%2F` = `/`
- `%3F` = `?`
- `%23` = `#`
- `%25` = `%`
- `%41` = `A`

### **When to Decode**

**DO decode**:
- After splitting into path and query
- Before using path for file access
- Before security validation

**DON'T decode**:
- Before splitting on `?` (query delimiter)
- Before splitting on `#` (fragment delimiter)
- More than once (prevents double-encoding attacks)

### **Reserved Characters**

Characters that have special meaning in URIs:
```
: / ? # [ ] @ ! $ & ' ( ) * + , ; =
```

These should remain percent-encoded in path components.

### **Unreserved Characters**

Characters that are always safe:
```
A-Z a-z 0-9 - . _ ~
```

These don't need percent-encoding and should be decoded if encountered.

### **Decoding Process**

For each `%XX` in the string:
1. Check next two characters are hex digits (0-9, A-F, a-f)
2. Convert hex to numeric value
3. Convert numeric value to character
4. Replace `%XX` with that character

Invalid encodings to reject:
- `%ZZ` (non-hex digits)
- `%1` (incomplete encoding)
- `%` at end of string

---

## Practical Examples

### **Example 1: Simple Path**
```
Input: GET /index.html HTTP/1.1

Parse:
1. Extract URI: /index.html
2. No '?' → query = undefined
3. No '%' → no decoding needed
4. No dots → no normalization needed
5. Validate: starts with '/', reasonable length
6. Result: path = "/index.html"
```

### **Example 2: Path with Query**
```
Input: GET /search?q=hello+world HTTP/1.1

Parse:
1. Extract URI: /search?q=hello+world
2. Split on '?': path = "/search", query = "q=hello+world"
3. No '%' in path → no decoding needed
4. Validate path
5. Result: path = "/search", query = "q=hello+world"
```

### **Example 3: Encoded Path**
```
Input: GET /my%20file.html HTTP/1.1

Parse:
1. Extract URI: /my%20file.html
2. No '?' → query = undefined
3. Decode %20 → space
4. Result: path = "/my file.html"
```

### **Example 4: Directory Traversal Attempt**
```
Input: GET /../../../etc/passwd HTTP/1.1

Parse:
1. Extract URI: /../../../etc/passwd
2. No '?' → query = undefined
3. No '%' → no decoding needed
4. Remove dot-segments:
   - Start: /../../../etc/passwd
   - Process '/..' → cannot go above root → /
   - Process '/..' → cannot go above root → /
   - Process '/..' → cannot go above root → /
   - Process '/etc' → /etc
   - Process '/passwd' → /etc/passwd
5. Result: path = "/etc/passwd"
6. Validate: check if /etc/passwd is within document root
7. If not within document root → REJECT (403 Forbidden)
```

### **Example 5: Complex Path**
```
Input: GET /docs/./api/../guide/intro.html?version=2 HTTP/1.1

Parse:
1. Extract URI: /docs/./api/../guide/intro.html?version=2
2. Split on '?': 
   - path = "/docs/./api/../guide/intro.html"
   - query = "version=2"
3. No '%' → no decoding needed
4. Remove dot-segments:
   - Start: /docs/./api/../guide/intro.html
   - Process '/docs' → /docs
   - Process '/.' → remove → /docs
   - Process '/api' → /docs/api
   - Process '/..' → go up → /docs
   - Process '/guide' → /docs/guide
   - Process '/intro.html' → /docs/guide/intro.html
5. Result: path = "/docs/guide/intro.html", query = "version=2"
```

---

## What to Store

### **Parsed Request Structure**

You need to store:
- **Method**: string (GET, POST, DELETE)
- **Path**: string (decoded and normalized)
- **Query**: string or undefined (raw, not parsed into key-value pairs)
- **Version**: string (HTTP/1.1)

Optional tracking:
- **Has query**: boolean (was `?` present?)
- **Original URI**: string (for logging)

You typically DON'T need to store:
- Scheme (not in request line)
- Authority (use `Host:` header)
- Fragment (not sent to server)
- Individual query parameters (parse when needed)

---

## Common Pitfalls

### **1. Decoding Too Early**
❌ **Wrong**: Decode first, then split on `?`
✅ **Right**: Split on `?` first, then decode

**Why**: `%3F` is an encoded `?` and should NOT split the URI.

### **2. Not Removing Dot-Segments**
❌ **Wrong**: Use path directly without normalization
✅ **Right**: Remove `.` and `..` segments first

**Why**: Allows directory traversal attacks.

### **3. Decoding Multiple Times**
❌ **Wrong**: Decode, store, decode again when using
✅ **Right**: Decode once, store decoded version

**Why**: `%2520` → `%20` → space (attacker bypasses filters).

### **4. Allowing Null Bytes**
❌ **Wrong**: Decode `%00` and continue
✅ **Right**: Reject any URI with `%00`

**Why**: Can terminate strings early, bypassing security checks.

### **5. No Length Limits**
❌ **Wrong**: Accept any URI length
✅ **Right**: Reject URIs over reasonable limit (2-8KB)

**Why**: DoS attacks with huge URIs.

### **6. Case-Sensitive Path Comparison**
❌ **Wrong**: Treat `/Path` and `/path` as different (on case-insensitive systems)
✅ **Right**: Normalize case based on your file system

**Why**: Can bypass access controls.

### **7. Ignoring Trailing Slashes**
❌ **Wrong**: Treat `/path` and `/path/` identically
✅ **Right**: Decide on a policy and stick to it

**Why**: Web servers often treat these differently (file vs directory).

---

## Quick Reference

### **Essential Steps**

1. ✅ Extract request-URI from request line
2. ✅ Strip fragment (if present)
3. ✅ Split on `?` into path and query
4. ✅ Percent-decode the path
5. ✅ Remove dot-segments from path
6. ✅ Validate for security
7. ✅ Use normalized path

### **Security Checklist**

- ✅ Remove dot-segments (`.` and `..`)
- ✅ Reject null bytes (`%00`)
- ✅ Decode only once
- ✅ Enforce length limits
- ✅ Validate final path is within document root
- ✅ Check for OS-specific reserved names
- ✅ Reject invalid percent-encodings

### **What NOT to Worry About**

- ❌ Scheme parsing (`http://`)
- ❌ Authority parsing (`host:port`)
- ❌ Userinfo handling (`user:pass@`)
- ❌ IPv6 literals (`[::1]`)
- ❌ Complex URI equivalence
- ❌ Relative reference resolution algorithms

---

## Testing Your Parser

### **Valid Inputs to Handle**
```
/index.html
/path/to/file
/path?query
/path?query=value&foo=bar
/my%20file.html
/docs/../guide/intro.html
/path/./file
/
```

### **Invalid Inputs to Reject**
```
/path%00.html          (null byte)
/../../../etc/passwd   (escapes document root)
/path%ZZ               (invalid encoding)
/path%1                (incomplete encoding)
<extremely long URI>   (DoS)
```

### **Edge Cases to Test**
```
/path?                 (empty query)
/path#                 (empty fragment - strip it)
//path                 (double slash)
/path//file            (double slash in middle)
/.                     (just current dir)
/..                    (just parent dir)
```

---

## Summary

### **Bottom Line**

For a basic HTTP webserver:

**YOU NEED**:
- Path parsing
- Query extraction
- Percent-decoding
- Dot-segment removal
- Security validation

**YOU DON'T NEED**:
- Full RFC 3986 compliance
- Scheme/authority parsing
- Complex normalization
- Relative reference resolution

**FOCUS ON SECURITY**:
- Directory traversal prevention
- Null byte rejection
- Path validation
- Length limits

The key is to parse safely, normalize properly, and validate thoroughly - not to implement every feature of the URI specification.

