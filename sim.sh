#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/sim"

PORT=9091

# Build WASM
./build.sh

# Kill any existing server on our port
lsof -ti:"$PORT" 2>/dev/null | xargs -r kill 2>/dev/null || true

# Start web server in background
python3 -m http.server "$PORT" &
SERVER_PID=$!
trap "kill $SERVER_PID 2>/dev/null" EXIT

echo "Serving simulator at http://localhost:$PORT"

# Open browser
open "http://localhost:$PORT" 2>/dev/null \
  || xdg-open "http://localhost:$PORT" 2>/dev/null \
  || echo "Open http://localhost:$PORT in your browser"

# Keep running until Ctrl-C
wait "$SERVER_PID"
