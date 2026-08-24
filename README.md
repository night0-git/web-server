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

# What could be improved
Because this project primarily serves educational purposes, its architecture is surely lacking, and there are several improvements that could be made for a production-ready server. Below are some issues I could address:
- Everything runs on one event loop thread.
- Logging causes significant bottlenecks in verbose mode.
- Each connection follows a relatively heavy, fixed syscall path.
- File descriptors are stored in a simple array because C does not have a built-in hash map structure.
- `prepare_headers()` is a very heavy operation. It does `open` + `stat`, and a lot of `snprintf` to build the headers.

Therefore, I think the server could be greatly improved with more CPU cores, more flexible connection handling, parsing optimizations and caching strategies.