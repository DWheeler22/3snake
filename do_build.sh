#!/bin/bash
cd /home/dj/PurpleTeam/3snake/3snake
make clean > /tmp/build_out.txt 2>&1
make >> /tmp/build_out.txt 2>&1
BUILD_CODE=$?

echo "=== BUILD OUTPUT ==="
cat /tmp/build_out.txt

if [ $BUILD_CODE -eq 0 ]; then
    echo ""
    echo "✓ Build successful"
    ls -lh 3snake
else
    echo ""
    echo "✗ Build failed with code $BUILD_CODE"
fi
