# What this is
An educational web server written from scratch in C, with an epoll event loop.

# Usage
## Build from source
```bash
# Compile
make clean && make release

# Run
./bin/web-server [OPTIONS]
```
**Options**:

```
-p, --port PORT       Port to open (default to 8080)
-d, --dir DIRECTORY   Root directory
-v, --verbose         Verbose logging (-vv for trace)
-h, --help            Show this help
```

**Example**:
```
./bin/web-server -p 3000 -d /var/www -v
```

# Benchmark
Below is a benchmark for the release build, with [wrk](https://github.com/wg/wrk) over local network:
```
$ wrk -t8 -c20000 -d30s --latency http://127.0.0.1:8080/
Running 30s test @ http://127.0.0.1:8080/
  8 threads and 20000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   183.43ms   15.75ms 251.02ms   95.90%
    Req/Sec    13.84k     7.61k   44.43k    59.91%
  Latency Distribution
     50%  184.75ms
     75%  188.23ms
     90%  193.31ms
     99%  200.84ms
  3187308 requests in 30.10s, 54.52GB read
Requests/sec: 105906.93
Transfer/sec:      1.81GB
```
For this benchmark, the server serves static HTML files from the [HTML5 UP](https://github.com/zce/html5up) repository, **20000** connections are maintained for 30 seconds, during which the server handles around **105k** requests per second and an average latency of **183ms**.

*Hardware specs: 13th Gen Intel Core i5-13500HX, 16GB RAM*


# What could be improved
Because this project primarily serves educational purposes, its architecture is surely lacking, and there are several improvements that could be made for a production-ready server. Below are some issues I could address:
- Everything runs on one event loop thread.
- Logging causes significant bottlenecks in verbose mode.
- Each connection follows a relatively heavy, fixed syscall path.
- The current implementation uses level-triggered epoll, while a more performant approach is edge-triggered epoll.
- File descriptors are stored in a simple array because C does not have a built-in hash map structure.
- `prepare_headers()` is a very heavy operation. It does `open` + `stat`, and a lot of `snprintf` to build the headers.

Therefore, I think the server could be greatly improved with more CPU cores, more flexible connection handling, parsing optimizations and caching strategies.