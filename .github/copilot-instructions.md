# Copilot Instructions — HTTP Server Project

## Project context
- Language: C++17
- Platform: Linux on WSL2 (POSIX sockets)
- Build: CMake
- Goal: concurrent HTTP server with GET/POST behavior as defined in SDD and backlog

## Hard constraints
- Use STL + POSIX socket API only
- Do not use web frameworks (no cpprest, Boost.Beast, Crow, Pistache)
- Concurrency must use std::thread
- Protect shared logging with std::mutex
- No sleep-based race-condition workarounds
- Keep parsing bounded (max request size 64 KiB)

## Architecture preferences
- Keep listener/accept loop separate from request handling and response building
- Prefer small focused functions
- Use RAII for client socket lifetime
- Pass accepted socket fd by value/move into thread workers

## Code style
- Clear names, minimal comments, no unnecessary abstractions
- Avoid giant functions
- Handle error paths explicitly and fail with clear messages

## Validation expectations
- Changes should preserve SDD requirements REQ-001..REQ-005
- Provide simple manual test commands (GET, POST, unsupported method)
- Keep README build and test instructions updated if behavior changes