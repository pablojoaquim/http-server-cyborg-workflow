# SDD — HTTP Server (C++17, POSIX, WSL2)

| Field | Value |
|-------|-------|
| **Project** | Concurrent HTTP Server (HTML + form) |
| **Version** | 0.1 |
| **Date** | 2026-06-24 |
| **C++ standard** | C++17 |
| **Toolchain** | GCC, WSL2 (Ubuntu), POSIX sockets |

---

## 1. Vision and scope

Build a TCP server that listens on port **8080**, accepts **concurrent connections** (one `std::thread` per connection), and handles a minimal subset of HTTP/1.1:

- `GET /` → responds with HTML page containing a name/surname form
- `POST /` → reads the body, logs it to the server console, responds with confirmation text
- Other methods → 405 or 501

**Out of scope:** HTTPS, keep-alive, frameworks (no cpprest, Boost.Beast, etc.), auth, databases.

### Measurable objectives

| ID | Objective | Verification |
|----|-----------|--------------|
| OBJ-001 | ≥2 parallel clients get responses without blocking each other | Two concurrent `curl` calls both complete |
| OBJ-002 | GET returns HTML with a form; POST logs body and confirms | Manual checklist |
| OBJ-003 | Code is C++, not C+pthreads | Code review |

---

## 2. Constraints

| ID | Constraint | Detail |
|----|-----------|--------|
| CTR-001 | Standard | C++17 minimum |
| CTR-002 | Platform | Linux / WSL2, POSIX sockets |
| CTR-003 | Libraries | STL + OS socket API only |
| CTR-004 | Concurrency | `std::thread` mandatory; `std::mutex` for shared state |
| CTR-005 | Latency | No `sleep` to "fix" race conditions |

---

## 3. Functional requirements

### REQ-001 — Listening socket and port

- **Description:** Create TCP socket, bind to `INADDR_ANY:8080`, listen. Port changeable via constant or CLI arg.
- **Priority:** Must
- **Acceptance criteria:**
  1. On startup, server prints the listening URL.
  2. If port is in use, program exits with a clear error message.

### REQ-002 — Concurrency per connection

- **Description:** After each `accept`, handle the client in a separate `std::thread`.
- **Priority:** Must
- **Acceptance criteria:**
  1. Two simultaneous clients both get responses without one waiting for the other.
  2. Client socket fd is passed **by value** to the thread (no pointer to local variable).

### REQ-003 — GET / with HTML form

- **Description:** `GET /` returns `200 OK`, `Content-Type: text/html`, body with a form (fields: first name, last name; submit via POST to `/`).
- **Priority:** Must
- **Acceptance criteria:**
  1. Opening URL in browser shows the form without protocol errors.

### REQ-004 — POST / with body

- **Description:** `POST /` reads headers + body (using `Content-Length`), logs body to server console, responds `200` with plain-text confirmation.
- **Priority:** Must
- **Acceptance criteria:**
  1. Submitting the form prints `fname=...&lname=...` to server console.
  2. Browser/curl receives readable confirmation response.

### REQ-005 — Unsupported methods

- **Description:** Any other method returns 405 or 501 with a short body.
- **Priority:** Should
- **Acceptance criteria:**
  1. `curl -X PUT http://127.0.0.1:8080/` returns 405/501 and server keeps running.

---

## 4. Non-functional requirements

| ID | Category | Requirement | How to verify |
|----|----------|-------------|---------------|
| NFR-001 | Maintainability | Separate: accept loop, request reading, response building | Code review |
| NFR-002 | Performance | Handle ≥16 concurrent connections without crash | Manual stress test |
| NFR-003 | Robustness | Buffer size capped; validate `Content-Length` (max 64 KiB) | curl test |
| NFR-004 | Build | Compiles with `-Wall -Wextra` without critical warnings | Build output |

---

## 5. Architecture

| Module | Responsibility | Suggested file |
|--------|---------------|----------------|
| `main` | Args, start/stop | `src/main.cpp` |
| `TcpListener` | socket, bind, listen, accept loop | `src/server.hpp/cpp` |
| `handleClient()` | Read request, dispatch GET/POST, write response | `src/http_handler.cpp` |
| `buildResponse()` | Assemble HTTP status + headers + body | `src/http_handler.hpp` |

### Architecture decisions

| ID | Decision | Discarded alternative | Reason |
|----|----------|-----------------------|--------|
| ARC-001 | One `std::thread` per connection | `select` single-thread | Pedagogical: learn threads |
| ARC-002 | Manual HTTP parsing | cpprest | Course constraint |
| ARC-003 | RAII for client socket | Manual close everywhere | Avoid fd leaks on error paths |

### Concurrency

| ID | Shared resource | Mechanism |
|----|-----------------|---------  |
| THR-001 | `std::cout`/`std::cerr` | `std::mutex` lock before every log write |

---

## 6. Verification

| ID | Level | What | Command |
|----|-------|------|---------|
| TST-001 | Manual | GET returns 200 + HTML | `curl -v http://127.0.0.1:8080/` |
| TST-002 | Manual | POST logs body + confirms | `curl -d "fname=Ana&lname=Lopez" -X POST http://127.0.0.1:8080/` |
| TST-003 | Manual | Concurrency | Two terminals, same curl at same time |
| TST-004 | Manual | Unknown method | `curl -X PUT http://127.0.0.1:8080/` |