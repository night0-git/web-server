# What this is
An educational web server written from scratch in C, with an epoll event loop.

# Usage
## Build from source
```bash
# Compile
make clean && make

# Run
./bin/web-server [OPTIONS]
```
**Options**:

```
-p, --port PORT       Port to open (default to 8080)
-d, --dir DIRECTORY   Root directory
-v, --verbose         Verbose logging (log incoming requests)
-h, --help            Show this help
```

**Example**:
```bash
./bin/web-server -p 3000 -d /var/www -v
```

# Features
- Central `epoll` event loop.
- Persistent connections (`HTTP/1.1 Keep-Alive`), client sockets are reused across requests.
- Handle `GET` requests, file extensions to MIME types mapping (`.html`, `.css`, `.png`, `.jpg`, `.ico`, `.pdf`).
- Zero-copy file transfer using Linux `sendfile` and `TCP_CORK`.

# Benchmarks
Below are benchmarking results performed on my laptop, using [wrk](https://github.com/wg/wrk). The server serves static HTML files from the [HTML5 UP](https://github.com/zce/html5up) repository, specifically the `aerial` template. Server runs without verbose flag, because logging is a major bottleneck in my implementation. Nginx baseline runs on a similar configuration with 1 worker process and logging disabled ([benchmarks/nginx.conf](benchmarks/nginx.conf)). Raw benchmark outputs are logged in [benchmarks/RESULTS.md](benchmarks/RESULTS.md).

> *Hardware specs: 13th Gen Intel Core i5-13500HX, 16GB RAM*


| Connections | Threads | Server | Throughput (req/s) | Avg Latency | p50 Latency | p99 Latency |
|:---:|:---:|:---|:---:|:---:|:---:|:---:|
| 100 | 4 | **This Server** | 187,396.16 | 532.95us | 519.00us | 759.00us |
| 100 | 4 | Nginx | 148,339.97 | 675.23us | 667.00us | 795.00us |
| 1,000 | 8 | **This Server** | 159,930.76 | 6.24ms | 6.08ms | 9.29ms |
| 1,000 | 8 | Nginx | 140,214.76 | 8.94ms | 7.14ms | 8.77ms |
| 4,000 | 8 | **This Server** | 125,226.33 | 31.84ms | 31.54ms | 35.79ms |
| 4,000 | 8 | Nginx | 120,634.38 | 31.46ms | 31.17ms | 35.44ms |


# Profiling
Profiling with `perf` at 1,000 concurrent connections ([perf/report.txt](perf/report.txt)) shows that **~87%** of CPU time is spent inside Linux system calls, with each request executing ~10 syscalls. Toggling `TCP_CORK` via `setsockopt` accounts for nearly half of all cycles because it triggers kernel TCP frame transmission and loopback networking:

| Syscall / Subsystem | % CPU Cycles | Role in Request Path |
| :--- | :---: | :--- |
| **`setsockopt`** | 47.7% | Toggling `TCP_CORK` (pushes frames and processes loopback ACKs) |
| **`sendfile`** | 8.7% | Zero-copy transmission of the static file body |
| **`epoll_ctl`** | 8.4% | Toggling connection flags (`EPOLLIN` $\leftrightarrow$ `EPOLLOUT`) |
| **`open` / `openat`** | 7.4% | Opening file descriptor per GET request |
| **`epoll_wait`** | 7.0% | Polling the event loop |
| **`write`** | 6.3% | Writing HTTP response headers to socket |
| **`read`** | 5.7% | Reading incoming HTTP request into buffer |

In user space (~13% total), `prepare_headers()` is the main consumer (9.5%), spent on `snprintf()` header formatting and path resolution.


# Limitations and next steps

### Known Limitations
- The server runs on a single event loop thread and cannot utilize multiple CPU cores. Throughput is bounded by the single-core execution speed.
- Each request incurs ~10 system calls. Repeatedly calling `open()` and `fstat()` per request consumes ~7.4% of CPU cycles that could be eliminated with metadata and descriptor caching.
- Verbose logging (`-v`) writes synchronously to `stdout`/`stderr`, blocking the event loop on slow terminal I/O and causing throughput drops under load.
- Persistent connections are not timed out. An idle client holds its descriptor slot in `clients[MAX_FDS]` indefinitely.
- Request parsing is naive and simple. URI paths are appended directly to the root directory without canonicalization, leaving the server without path traversal (`../`) filtering or TLS/HTTPS support.

### Future Work
- Use `SO_REUSEPORT` to distribute incoming connections across multiple worker processes or threads to saturate all CPU cores.
- Implement an in-memory LRU cache for open file descriptors and pre-computed HTTP headers (Content-Length, MIME type) to bypass repeated `open()`, `fstat()`, and `snprintf()` calls.
- Migrate client sockets to edge-triggered mode to reduce repeated `epoll_ctl` toggling between `EPOLLIN` and `EPOLLOUT`.
- Enforce idle keep-alive timeouts and request read timeouts, preventing connection slot exhaustion.
