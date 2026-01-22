#!/bin/bash

# Build
cd /home/dj/PurpleTeam/3snake/3snake
echo "[*] Building..."
make clean > /tmp/build.log 2>&1 && make >> /tmp/build.log 2>&1
if [ $? -ne 0 ]; then
    echo "Build failed!"
    cat /tmp/build.log
    exit 1
fi
echo "[✓] Build successful"

# Clean output
rm -f /tmp/tracer_output.txt

# Start tracer in background
echo "[*] Starting 3snake tracer..."
timeout 20 sudo ./3snake -o /tmp/tracer_output.txt > /dev/null 2>&1 &
TRACER_PID=$!
sleep 2

# Test 1: passwd
echo "[*] Test 1: Running 'passwd root' with input 'testpass123'"
printf "testpass123\n" | sudo passwd root 2>/dev/null || true
sleep 2

# Kill tracer
kill $TRACER_PID 2>/dev/null || true
sleep 1

# Show results
echo ""
echo "=== TRACER OUTPUT ==="
if [ -f /tmp/tracer_output.txt ]; then
    cat /tmp/tracer_output.txt
else
    echo "No output file found"
fi

echo ""
echo "=== ANALYSIS ==="
if grep -q "\[CRED\] Captured" /tmp/tracer_output.txt 2>/dev/null; then
    echo "[✓] Credentials captured!"
    grep "\[CRED\]" /tmp/tracer_output.txt
else
    echo "[✗] No credentials captured"
fi

if grep -q "testpass123" /tmp/tracer_output.txt 2>/dev/null; then
    echo "[✓] Password output found!"
else
    echo "[?] Password not in output - checking what was captured..."
    grep -E "\[TRACE\]|type=" /tmp/tracer_output.txt | head -10
fi
