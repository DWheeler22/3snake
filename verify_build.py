#!/usr/bin/env python3
import subprocess
import os
import sys

os.chdir('/home/dj/PurpleTeam/3snake/3snake')

print("Cleaning old build...")
result = subprocess.run(['make', 'clean'], capture_output=True, text=True, timeout=10)

print("Building...")
result = subprocess.run(['make'], capture_output=True, text=True, timeout=30)

if result.returncode == 0:
    print("✓ Build successful!")
    result = subprocess.run(['ls', '-lh', '3snake'], capture_output=True, text=True)
    print(result.stdout)
else:
    print("✗ Build failed!")
    print("STDOUT:", result.stdout[-500:] if len(result.stdout) > 500 else result.stdout)
    print("STDERR:", result.stderr[-500:] if len(result.stderr) > 500 else result.stderr)
    sys.exit(1)
