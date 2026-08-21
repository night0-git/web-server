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

-p, --port PORT         Port to open (default to 8080) \
-d, --dir DIRECTORY     Root directory \
-v, --verbose           Verbose logging (-vv for trace) \
-h, --help              Show this help

**Example**:
```
./bin/web-server -p 3000 -d /var/www -v
```