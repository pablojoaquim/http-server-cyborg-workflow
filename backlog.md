# Backlog — Concurrent HTTP Server (C++17, POSIX, WSL2)

**SDD reference:** `docs/sdd.md`

| Field | Value |
|-------|-------|
| **Project** | Concurrent HTTP Server |
| **Status** | Planning |

**Status legend:** `TODO` · `Doing` · `Done` · `Blocked`

| ID | Epic | Title |
|----|------|-------|
| TASK-1  | TCP Infrastructure | Configure CMake and C++17 standard |
| TASK-2  | TCP Infrastructure | Platform spike — POSIX sockets on WSL2 |
| TASK-3  | TCP Infrastructure | Implement listening socket, bind, listen, port |
| TASK-4  | TCP Infrastructure | RAII for client socket and safe close |
| TASK-5  | TCP Infrastructure | Delegate each accepted connection to a std::thread |
| TASK-6  | TCP Infrastructure | Synchronize concurrent logging with std::mutex |
| TASK-7  | HTTP + Quality     | Read HTTP request with bounded buffer, delimit headers |
| TASK-8  | HTTP + Quality     | Respond to GET with HTML and name/surname form |
| TASK-9  | HTTP + Quality     | Implement POST — read body and confirm |
| TASK-10 | HTTP + Quality     | Return 405/501 for unsupported methods |
| TASK-11 | HTTP + Quality     | Modular refactor — separate listener, handler, response builder |
| TASK-12 | Delivery           | Complete README with curl commands and platform |
| TASK-13 | Delivery           | Concurrency and light stress test (≥16 connections) |
| TASK-14 | Delivery           | Final checklist against SDD |

---

## Epic: TCP Infrastructure

**Description:** Compilable C++17 executable, POSIX socket layer, listening socket, one std::thread per connection, RAII on client socket, synchronized logging. Epic closes when a client receives a minimal HTTP response from concurrent threads.

**SDD traceability:** REQ-001, REQ-002, OBJ-001, OBJ-003, CTR-001–CTR-004, THR-001, NFR-004

**Epic done criteria:**
- `cmake --build` with `-Wall -Wextra` has no errors
- Two parallel `curl` calls both get a response without indefinite blocking
- No obvious fd leaks after several connections

**Epic status:** `TODO`

---

### TASK-1 — Configure CMake and C++17 standard

**Objective:** Have a compilable executable target on WSL2 with warning flags.

**Context:** CTR-001, NFR-004. Link `-lpthread` if needed.

**Suggested steps:**
1. Create `CMakeLists.txt` with `set(CMAKE_CXX_STANDARD 17)` and target `http_server`
2. Add `-Wall -Wextra` flags for GCC
3. Document one-line build command in README

**Acceptance criteria:**
- Project compiles from scratch on WSL2
- Standard is C++17

**Dependencies:** None — **Status:** `TODO`

---

### TASK-2 — Platform spike — POSIX sockets on WSL2

**Objective:** Confirm which headers, types and link flags are needed on this platform.

**Context:** CTR-002, CTR-003. Headers: `<sys/socket.h>`, `<netinet/in.h>`, `<unistd.h>`. Type: `int` for fd.

**Suggested steps:**
1. Write a minimal `socket()` + `close()` program that compiles and exits cleanly
2. Document the result (headers, any flags) as a comment in `main.cpp`

**Acceptance criteria:**
- Minimal socket program compiles and runs without errors on WSL2

**Dependencies:** TASK-1 — **Status:** `TODO`

---

### TASK-3 — Listening socket, bind, listen, port

**Objective:** Server opens a TCP socket on port 8080 and prints the listening URL.

**Context:** REQ-001. Use `SO_REUSEADDR`. Print `http://127.0.0.1:8080` on start. Exit with clear message if port busy.

**Suggested steps:**
1. `socket()` → `setsockopt(SO_REUSEADDR)` → `bind()` → `listen()`
2. Print URL on success; check return values and exit with `perror` on failure
3. Block on `accept()` (no threading yet)

**Acceptance criteria:**
1. Server prints URL on startup
2. If port is in use, program exits with clear error message

**Dependencies:** TASK-2 — **Status:** `TODO`

---

### TASK-4 — RAII for client socket and safe close

**Objective:** Wrap the client fd in a RAII class so it closes on any exit path.

**Context:** ARC-003, REQ-002. Classic bug: forgetting `close()` in error paths. A small class with destructor solves it.

**Suggested steps:**
1. Create `class ClientSocket` with `int fd`; destructor calls `close(fd)` if valid
2. Non-copyable (delete copy constructor/assignment); movable optional
3. Replace manual `close()` calls with this wrapper

**Acceptance criteria:**
- No fd leak when handler returns early on parse error

**Dependencies:** TASK-3 — **Status:** `TODO`

---

### TASK-5 — Delegate each connection to a std::thread

**Objective:** Accept loop spawns a `std::thread` per connection; main thread keeps accepting.

**Context:** REQ-002, ARC-001, THR-001. Pass fd **by value** into the lambda — classic bug is passing pointer to local variable.

**Suggested steps:**
1. Wrap accepted fd in `ClientSocket`, pass it by move into `std::thread` lambda
2. Call `thread.detach()` (document why vs join)
3. Verify main loop continues accepting while handler runs

**Acceptance criteria:**
1. Two simultaneous `curl` calls both complete
2. fd is passed by value/move, not as pointer to stack variable

**Dependencies:** TASK-4 — **Status:** `TODO`

---

### TASK-6 — Synchronize concurrent logging with std::mutex

**Objective:** All log writes to `std::cout`/`std::cerr` are protected by a single mutex.

**Context:** THR-001, CTR-004. Without mutex, interleaved output is a data race (UB).

**Suggested steps:**
1. Declare `std::mutex g_log_mutex` (global or passed by ref)
2. Create a small `log(std::string)` helper that locks before writing
3. Replace all direct `cout`/`cerr` calls with this helper

**Acceptance criteria:**
- No garbled/interleaved output with two concurrent clients

**Dependencies:** TASK-5 — **Status:** `TODO`

---

## Epic: HTTP + Quality

**Description:** Parse incoming HTTP requests, dispatch GET/POST, build well-formed HTTP responses, return 405/501 for other methods, then refactor into clean modules.

**SDD traceability:** REQ-003, REQ-004, REQ-005, NFR-001, NFR-003

**Epic status:** `TODO`

---

### TASK-7 — Read HTTP request with bounded buffer, delimit headers

**Objective:** Read raw bytes from socket into a capped buffer; extract method, path, headers, body.

**Context:** NFR-003. Buffer max: 64 KiB. Split on `\r\n\r\n` to separate headers from body. Read body using `Content-Length`.

**Suggested steps:**
1. `recv()` into `std::array<char, 65536>` 
2. Find `\r\n\r\n`; parse first line into method + path + version
3. Parse headers into a simple map; read body bytes using `Content-Length`

**Acceptance criteria:**
- Function returns a struct/object with method, path, headers map, body string
- Does not read beyond `Content-Length` bytes

**Dependencies:** TASK-6 — **Status:** `TODO`

---

### TASK-8 — Respond to GET with HTML and name/surname form

**Objective:** `GET /` returns `200 OK` with an HTML page containing a form.

**Context:** REQ-003. Form fields: `fname`, `lname`. Method POST, action `/`. Build response as string with `\r\n` line endings.

**Suggested steps:**
1. Build response string: status line + headers + blank line + HTML body
2. Include `Content-Length` header with correct byte count
3. `send()` the full response string

**Acceptance criteria:**
1. `curl -v http://127.0.0.1:8080/` returns `200` and HTML
2. Opening in browser shows the form

**Dependencies:** TASK-7 — **Status:** `TODO`

---

### TASK-9 — Implement POST — read body and confirm

**Objective:** `POST /` logs the body to console and responds with plain-text confirmation.

**Context:** REQ-004. Body is `application/x-www-form-urlencoded`, e.g. `fname=Ana&lname=Lopez`.

**Suggested steps:**
1. If method is POST, log body with mutex-protected logger
2. Build `200 OK` response with `Content-Type: text/plain` and confirmation message
3. `send()` response

**Acceptance criteria:**
1. Server console shows `fname=...&lname=...` after form submit
2. Browser/curl receives readable confirmation

**Dependencies:** TASK-8 — **Status:** `TODO`

---

### TASK-10 — Return 405/501 for unsupported methods

**Objective:** Any method other than GET/POST returns 405 or 501 and server keeps running.

**Context:** REQ-005.

**Suggested steps:**
1. Add else branch in method dispatcher
2. Build and send `405 Method Not Allowed` response with short body

**Acceptance criteria:**
- `curl -X PUT http://127.0.0.1:8080/` returns 405/501 and server accepts next request

**Dependencies:** TASK-9 — **Status:** `TODO`

---

### TASK-11 — Modular refactor

**Objective:** Separate accept loop, request reading, and response building into distinct functions/files.

**Context:** NFR-001. No new features — reorganize only.

**Suggested steps:**
1. Move HTTP parsing to `http_handler.cpp` / `http_handler.hpp`
2. Move response builders to `http_response.hpp`
3. `main.cpp` only contains the accept loop and thread spawn

**Acceptance criteria:**
- All previous tasks still pass after refactor
- Each file has a single clear responsibility

**Dependencies:** TASK-10 — **Status:** `TODO`

---

## Epic: Delivery

**Epic status:** `TODO`

---

### TASK-12 — Complete README with curl commands and platform

**Objective:** README documents how to build and test the server.

**Suggested steps:**
1. Fill in the Build section with cmake commands
2. Add curl examples for GET, POST, and PUT (405 test)
3. Add WSL2-specific note (port forwarding to Windows browser)

**Dependencies:** TASK-11 — **Status:** `TODO`

---

### TASK-13 — Concurrency and light stress test (≥16 connections)

**Objective:** Server handles ≥16 simultaneous connections without crash.

**Suggested steps:**
1. Run: `for i in $(seq 1 16); do curl -s http://127.0.0.1:8080/ & done; wait`
2. Verify all return HTML and no crash/hang

**Acceptance criteria:**
- All 16 requests complete; server still responsive after

**Dependencies:** TASK-12 — **Status:** `TODO`

---

### TASK-14 — Final checklist against SDD

**Objective:** Verify every SDD requirement is met.

**Suggested steps:**
1. Go through REQ-001 to REQ-005 and OBJ-001 to OBJ-003 one by one
2. Run TST-001 to TST-004 from SDD section 6
3. Mark each as Done or note gaps

**Dependencies:** TASK-13 — **Status:** `TODO`