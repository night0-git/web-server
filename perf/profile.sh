#!/usr/bin/env bash
set -euo pipefail

# Find running web-server PID
SERVER_PID=$(pgrep -x web-server || true)
if [ -z "$SERVER_PID" ]; then
    echo "Error: 'web-server' is not running."
    echo "Please start the server first in another terminal, e.g.:"
    echo "  ./bin/web-server -p 8080 -d /home/night0/Downloads/html5up/aerial"
    exit 1
fi

URL="http://127.0.0.1:8080/"
DURATION=25       # Total wrk test duration in seconds
PROFILE_TIME=15   # How long perf samples steady state
OUTPUT_FILE="perf.data"

echo "==> Detected web-server PID: $SERVER_PID"
echo "==> Warming up for 3 seconds..."
wrk -t4 -c100 -d3s "$URL" > /dev/null 2>&1 || true

echo "==> Starting wrk (${DURATION}s, 1000 connections)..."
wrk -t8 -c1000 -d"${DURATION}s" "$URL" > /tmp/wrk_output.txt &
WRK_PID=$!

# Wait 3s to let wrk ramp up and establish connections
echo "==> Waiting 3s for steady-state traffic..."
sleep 3

echo "==> Recording perf for ${PROFILE_TIME}s into ${OUTPUT_FILE}..."
sudo perf record -F 99 -g -p "$SERVER_PID" -o "$OUTPUT_FILE" -- sleep "$PROFILE_TIME"
sudo chmod 644 "$OUTPUT_FILE"

echo "==> Waiting for wrk to finish..."
wait "$WRK_PID"

echo ""
echo "================= wrk Summary ================="
cat /tmp/wrk_output.txt
echo "==============================================="
echo ""
echo "==> Done. View the results with:"
echo "    sudo perf report"
