# Benchmark Raw Results

**Hardware Specs**: 13th Gen Intel Core i5-13500HX, 16GB RAM  
**Target File**: Static HTML files from [HTML5 UP](https://github.com/zce/html5up) (`aerial` template)  
**Tool**: [wrk](https://github.com/wg/wrk) (HTTP/1.1 keep-alive enabled)

---

## 1. Custom Web Server (C, single event loop thread)

### 100 Connections (4 Threads, 30s)
```bash
$ wrk -t4 -c100 -d30s --latency http://127.0.0.1:8080/
```
```
Running 30s test @ http://127.0.0.1:8080/
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   532.95us   73.75us   5.77ms   94.87%
    Req/Sec    47.10k     1.85k   51.23k    67.67%
  Latency Distribution
     50%  519.00us
     75%  547.00us
     90%  570.00us
     99%  759.00us
  5623566 requests in 30.01s, 9.58GB read
Requests/sec: 187396.16
Transfer/sec:    326.87MB
```

### 1,000 Connections (8 Threads, 30s)
```bash
$ wrk -t8 -c1000 -d30s --latency http://127.0.0.1:8080/
```
```
Running 30s test @ http://127.0.0.1:8080/
  8 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     6.24ms  746.07us  16.46ms   89.62%
    Req/Sec    20.11k     1.10k   22.73k    56.00%
  Latency Distribution
     50%    6.08ms
     75%    6.36ms
     90%    7.02ms
     99%    9.29ms
  4805816 requests in 30.05s, 8.19GB read
Requests/sec: 159930.76
Transfer/sec:    278.96MB
```

### 4,000 Connections (8 Threads, 30s)
```bash
$ wrk -t8 -c4000 -d30s --latency http://127.0.0.1:8080/
```
```
Running 30s test @ http://127.0.0.1:8080/
  8 threads and 4000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    31.84ms    1.16ms  41.02ms   76.03%
    Req/Sec    15.78k     1.17k   20.20k    80.67%
  Latency Distribution
     50%   31.54ms
     75%   32.39ms
     90%   33.35ms
     99%   35.79ms
  3768440 requests in 30.09s, 6.42GB read
Requests/sec: 125226.33
Transfer/sec:    218.43MB
```

### Previous 20,000 Connections Run (8 Threads, 30s)
```bash
$ wrk -t8 -c20000 -d30s --latency http://127.0.0.1:8080/
```
```
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

---

## 2. Nginx Baseline (1 worker process, access_log off)

Command:
```bash
nginx -p <serving_dir> -c "$PWD/benchmarks/nginx.conf"
```

### 100 Connections (4 Threads, 30s)
```bash
$ wrk -t4 -c100 -d30s --latency http://127.0.0.1:8080/
```
```
Running 30s test @ http://127.0.0.1:8080/
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   675.23us  214.75us  30.60ms   99.63%
    Req/Sec    37.28k     1.58k   40.42k    62.00%
  Latency Distribution
     50%  667.00us
     75%  699.00us
     90%  722.00us
     99%  795.00us
  4451187 requests in 30.01s, 8.30GB read
Requests/sec: 148339.97
Transfer/sec:    283.22MB
```

### 1,000 Connections (8 Threads, 30s)
```bash
$ wrk -t8 -c1000 -d30s --latency http://127.0.0.1:8080/
```
```
Running 30s test @ http://127.0.0.1:8080/
  8 threads and 1000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     8.94ms   31.32ms   1.05s    99.51%
    Req/Sec    17.63k     1.01k   24.21k    75.96%
  Latency Distribution
     50%    7.14ms
     75%    7.26ms
     90%    7.58ms
     99%    8.77ms
  4212777 requests in 30.05s, 7.85GB read
Requests/sec: 140214.76
Transfer/sec:    267.71MB
```

### 4,000 Connections (8 Threads, 30s)
```bash
$ wrk -t8 -c4000 -d30s --latency http://127.0.0.1:8080/
```
```
Running 30s test @ http://127.0.0.1:8080/
  8 threads and 4000 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    31.46ms   20.61ms   1.98s    99.93%
    Req/Sec    15.19k     2.06k   20.16k    70.08%
  Latency Distribution
     50%   31.17ms
     75%   33.32ms
     90%   33.72ms
     99%   35.44ms
  3628011 requests in 30.07s, 6.76GB read
  Socket errors: connect 0, read 0, write 0, timeout 664
Requests/sec: 120634.38
Transfer/sec:    230.32MB
```
