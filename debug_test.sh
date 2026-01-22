#!/bin/bash
# Quick test with credential capture debugging

cd /home/dj/PurpleTeam/3snake/3snake

echo "[*] Testing 3snake with credential debugging"
echo "[*] Press Ctrl+C to stop"
echo ""

# Start tracer in background with 30 second timeout
timeout 30 sudo ./3snake 2>&1 | tee /tmp/3snake_test_output.txt &
TRACER_PID=$!

# Wait for tracer to start
sleep 2

# Test with sudo
echo "[*] Testing: sudo -l"
echo "your_password_here" | sudo -l 2>/dev/null || true
sleep 1

# Test with su  
echo "[*] Testing: su -c whoami"
echo "your_password_here" | sudo su -c whoami 2>/dev/null || true
sleep 1

# Wait for tracer to finish
wait $TRACER_PID 2>/dev/null || true

echo ""
echo "[*] Output captured:"
cat /tmp/3snake_test_output.txt | grep -E "\[CRED\]|\[TRACE\]|Tracing"
